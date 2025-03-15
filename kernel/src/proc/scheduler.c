#include <proc/scheduler.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <dev/stdout.h>
#include <lib/spinlock.h>
#include <proc/user.h>

pcb_t **procs;
uint64_t count = 0;
uint64_t current_pid = 0;
spinlock_t lock = SPINLOCK_INIT;

void map_range_to_pagemap(uint64_t *dest_pagemap, uint64_t *src_pagemap, uint64_t start, uint64_t size, uint64_t flags)
{
    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE)
    {
        uint64_t phys = virt_to_phys(src_pagemap, start + offset);
        if (phys)
        {
            vmm_map(dest_pagemap, start + offset, phys, flags);
        }
    }
}

void scheduler_init()
{
    // Use a more efficient memory allocation
    procs = (pcb_t **)kcalloc(sizeof(pcb_t *), PROC_MAX_PROCS);
    if (!procs)
    {
        error("Failed to initialize scheduler process list");
        return;
    }

    trace("Initialized scheduler process list, %d bytes (%d max processes)", sizeof(pcb_t *) * PROC_MAX_PROCS, PROC_MAX_PROCS);
}

uint64_t scheduler_spawn(void (*entry)(void), uint64_t *pagemap)
{
    pcb_t *proc = (pcb_t *)kmalloc(sizeof(pcb_t));
    if (!proc)
    {
        error("Failed to allocate memory for new process");
        return -1;
    }

    proc->pid = count++;
    proc->state = PROCESS_READY;
    proc->ctx.rip = (uint64_t)entry;
    proc->ctx.rsp = (uint64_t)HIGHER_HALF(pmm_request_page() + 4095); // Account for stack growing downwards.
    proc->ctx.cs = 0x08;
    proc->ctx.ss = 0x10;
    proc->ctx.rflags = 0x202;
    proc->pagemap = pagemap;
    vmm_map(proc->pagemap, (uint64_t)proc, (uint64_t)proc, VMM_PRESENT | VMM_WRITE);
    map_range_to_pagemap(proc->pagemap, kernel_pagemap, (uint64_t)procs, sizeof(pcb_t *) * PROC_MAX_PROCS, VMM_PRESENT | VMM_WRITE);
    map_range_to_pagemap(proc->pagemap, kernel_pagemap, 0x1000, 0x10000, VMM_PRESENT | VMM_WRITE);
    proc->timeslice = PROC_DEFAULT_TIME;
    proc->errno = EOK;
    proc->whoami = 0; // run as root by default

    proc->fd_count = 0;
    proc->fd_table = (vnode_t **)kmalloc(sizeof(vnode_t *) * PROC_MAX_FDS);
    if (!proc->fd_table)
    {
        error("Failed to allocate memory for file descriptor table");
        kfree(proc);
        return -1;
    }

    for (int i = 0; i < PROC_MAX_FDS; i++)
    {
        proc->fd_table[i] = NULL;
    }

    procs[proc->pid] = proc;

    // Setup default file descriptor table.
    // - 0: stdout
    scheduler_proc_add_vnode(proc->pid, stdout);

    trace("Spawned process %d with entry %p, and pagemap %p", proc->pid, entry, pagemap);
    return proc->pid;
}

void scheduler_tick(struct register_ctx *ctx)
{
    spinlock_acquire(&lock);
    if (count == 0)
    {
        spinlock_release(&lock);
        return;
    }

    pcb_t *proc = procs[current_pid];
    if (proc && proc->state == PROCESS_RUNNING)
    {
        memcpy(&proc->ctx, ctx, sizeof(struct register_ctx));

        if (--proc->timeslice == 0)
        {
            proc->state = PROCESS_READY;
            proc->timeslice = PROC_DEFAULT_TIME;
            current_pid = (current_pid + 1) % count;
        }
    }

    pcb_t *next_proc = procs[current_pid];
    assert(ctx && next_proc);
    if (next_proc)
    {
        if (next_proc->state == PROCESS_READY)
        {
            next_proc->state = PROCESS_RUNNING;
            assert(next_proc->pagemap);
            assert(ctx);
            assert(&next_proc->ctx);
            memcpy(ctx, &next_proc->ctx, sizeof(struct register_ctx));
            vmm_switch_pagemap(next_proc->pagemap);
        }
        else if (next_proc->state == PROCESS_TERMINATED)
        {
            procs[next_proc->pid] = NULL;
            count--;

            if (count == 0)
                spinlock_release(&lock);
            {
                return;
            }

            current_pid = (current_pid + 1) % count;
        }
    }
    spinlock_release(&lock);
}

void scheduler_exit(int return_code)
{
    (void)return_code; // might be unused.
    pcb_t *proc = procs[current_pid];
    if (proc)
    {
        proc->ctx.rip = 0;

        for (uint64_t i = 0; i < proc->fd_count; i++)
        {
            if (proc->fd_table[i] != NULL)
            {
                proc->fd_table[i] = NULL;
            }
        }

        vmm_destroy_pagemap(proc->pagemap);
        kfree(proc->fd_table);
        kfree(proc);

        procs[proc->pid] = NULL;
        count--;

        current_pid = (count == 0) ? 0 : (current_pid + 1) % count;
        trace("Process %d exited with return code %d", proc->pid, return_code);
    }
    else
    {
        error("No process to exit (current_pid: %d)", current_pid);
    }
}

pcb_t *scheduler_get_current()
{
    return procs[current_pid];
}

int scheduler_proc_add_vnode(uint64_t pid, vnode_t *node)
{
    if (pid >= count || procs[pid] == NULL)
    {
        error("Invalid pid %d for process", pid);
        return -1;
    }

    pcb_t *proc = procs[pid];
    assert(proc);
    assert(node);

    if (proc->fd_count < PROC_MAX_FDS)
    {
        proc->fd_table[proc->fd_count++] = node;
        trace("Added %s to fd %d in pid %d", vfs_get_full_path(node), proc->fd_count - 1, proc->pid);
    }
    else
    {
        error("No available file descriptors for process %d", proc->pid);
        return -1;
    }

    return proc->fd_count - 1;
}

int scheduler_proc_remove_vnode(uint64_t pid, int fd)
{
    if (pid >= count || procs[pid] == NULL)
    {
        error("Invalid pid %d for process", pid);
        return -2;
    }

    pcb_t *proc = procs[pid];
    assert(proc);
    trace("Attempting to remove fd: %d, pid: %d", fd, proc->pid);

    if (fd < 0 || fd >= PROC_MAX_FDS || proc->fd_table[fd] == NULL)
    {
        error("Invalid file descriptor %d for process %d", fd, pid);
        return -1;
    }

    if (proc->fd_table[fd] != NULL)
    {
        trace("Removing %s from fd %d in pid %d", vfs_get_full_path(proc->fd_table[fd]), fd, proc->pid);

        for (uint64_t i = fd; i < proc->fd_count - 1; i++)
        {
            proc->fd_table[i] = proc->fd_table[i + 1];
        }

        proc->fd_table[--proc->fd_count] = NULL;
    }
    return 0;
}
