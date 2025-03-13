#include <proc/scheduler.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

pcb_t **procs;
uint64_t count = 0;
uint64_t current_pid = 0;

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
    // Preallocate pcb_t to avoid repeated calls to kmalloc.
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
    proc->timeslice = PROC_DEFAULT_TIME;

    procs[proc->pid] = proc;

    trace("Spawned process %d with entry %p", proc->pid, entry);
    return proc->pid;
}

void scheduler_tick(struct register_ctx *ctx)
{
    if (count == 0)
        return;

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
    if (next_proc)
    {
        if (next_proc->state == PROCESS_READY)
        {
            next_proc->state = PROCESS_RUNNING;
            memcpy(ctx, &next_proc->ctx, sizeof(struct register_ctx));
            trace("pid %d be switching to pagemap: 0x%.16llx", next_proc->pid, (uint64_t)next_proc->pagemap);
            vmm_switch_pagemap(next_proc->pagemap);
        }
        else if (next_proc->state == PROCESS_TERMINATED)
        {
            trace("Process %d terminated, freeing resources", next_proc->pid);
            vmm_destroy_pagemap(next_proc->pagemap);
            kfree(next_proc);

            procs[next_proc->pid] = NULL;
            count--;

            if (count == 0)
            {
                return;
            }

            current_pid = (current_pid + 1) % count;
        }
    }
}

void scheduler_terminate(uint64_t pid)
{
    if (pid >= count || procs[pid] == NULL)
    {
        error("Attempted to terminate an invalid or non-existent process (pid: %d)", pid);
        return;
    }

    pcb_t *proc = procs[pid];
    if (proc->state == PROCESS_TERMINATED)
    {
        warning("Process %d is already terminated", pid);
        return;
    }

    trace("Terminating process %d", pid);

    // Efficient process cleanup.
    vmm_destroy_pagemap(proc->pagemap);
    kfree(proc);

    procs[pid] = NULL;
    count--;

    // Reset PID after termination.
    current_pid = (count == 0) ? 0 : (current_pid + 1) % count;
}

void scheduler_exit(int return_code)
{
    pcb_t *proc = procs[current_pid];
    if (proc)
    {
        trace("Process %d exiting with return code %d", proc->pid, return_code);

        proc->state = PROCESS_TERMINATED;
        proc->ctx.rip = 0;

        warning("pid %d got brutally killed :( (%d)\n", proc->pid, return_code);

        scheduler_terminate(proc->pid);
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
