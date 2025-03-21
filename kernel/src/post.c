#include <dev/vfs.h>
#include <lib/printf.h>
#include <dev/stdout.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <dev/timer/pit.h>
#include <proc/scheduler.h>
#include <dev/portio.h>
#include <proc/data/elf.h>
#include <sys/gdt.h>
#include <dev/input/keyboard.h>
#include <dev/time/rtc.h>
#include <sys/syscall.h>

#define GET_KERNEL_CONFIG_VALUE(buff, key) ({ \
    char *value = NULL;                       \
    char *line = strtok(buff, "\n");          \
    while (line)                              \
    {                                         \
        char *temp = strstr(line, key "=");   \
        if (temp)                             \
        {                                     \
            value = temp + strlen(key) + 1;   \
            break;                            \
        }                                     \
        line = strtok(NULL, "\n");            \
    }                                         \
    value;                                    \
})

void final_debug()
{
    printf("\n");
    printf("\033[90m");
    printf("------------------------------------------------------------\n");
    printf("Finished running shadowOS, now we halt :^)\n\n");
    vnode_t *current = VFS_ROOT()->child;
    vnode_t *stack[256];
    int stack_depth = 0;
    assert(current);
    assert(current->name);

    while (current != NULL || stack_depth > 0)
    {
        if (current != NULL)
        {
            if (current->type != VNODE_DIR)
                VFS_PRINT_VNODE(current);
            if (current->child != NULL)
            {
                stack[stack_depth++] = current->next;
                current = current->child;
            }
            else
            {
                current = current->next;
            }
        }
        else
        {
            current = stack[--stack_depth];
        }
    }
    printf("\n");

    uint64_t free = pmm_get_free_memory();
    uint64_t total = pmm_get_total_memory();
    printf("Free memory:\t%llu MB\nTotal memory:\t%llu MB\n", BYTES_TO_MB(free), BYTES_TO_MB(total));
    printf("------------------------------------------------------------\n");
    printf("\n");
    printf("\033[0m");
}

void final()
{
    info("Finished running shadowOS");
}

void test_task()
{
    syscall(SYS_ioctl, 0, 0x0000, 0x0000); // ioctl(stdout, 0, 0)
    scheduler_exit(0);
}

extern uint64_t kernel_stack_top;
void post_main()
{
    info("shadowOS Kernel v1.0 successfully initialized at %s", TIMESTAMP_TO_STRING(GET_CURRENT_UNIX_TIME(), 0));
    assert(VFS_ROOT());
    assert(VFS_ROOT()->child);

    // Clear COM1 terminal, i got OSD or sum shit
    outb(0x3F8, '\033');
    outb(0x3F8, 'c');

    // Setup some kernel config stuff
    char *conf = VFS_READ("/etc/user.conf");
    bool def = false;
    if (conf == NULL)
    {
        warning("Failed to find \"/etc/user.conf\", is this intentional? Will use default config.");
        def = true;
    }

    char *init_path = DEFAULT_INIT_PROC_PATH;
    if (!def)
    {
        init_path = GET_KERNEL_CONFIG_VALUE(conf, "init");
        if (init_path == NULL)
        {
            warning("Failed to find a \"init\" feild in the /etc/user.conf file, is this intentional? Will use default init path");
            init_path = DEFAULT_INIT_PROC_PATH;
        }
    }

    // Initialize tss
    tss_init(kernel_stack_top);

    // Finish and spawn init task
    scheduler_init();
    // scheduler_spawn(false, test_task, vmm_new_pagemap());

    // Load init proc, in usermode
    info("Launching %s as init proc", init_path);
    vnode_t *init = vfs_lazy_lookup(VFS_ROOT()->mount, init_path);
    if (init == NULL)
    {
        error("\"%s\" missing. Did you bootstrap correctly or is your /etc/user.conf wrong? Check your initramfs. (Halting System)", init_path);
        hcf();
    }

    char *buf = (char *)kmalloc(init->size);
    assert(buf);
    vfs_read(init, buf, init->size, 0);
    VFS_READ(DEFAULT_INIT_PROC_PATH);
    uint64_t *pm = vmm_new_pagemap();
    trace("Loaded new pagemap at 0x%.16llx", (uint64_t)pm);
    uint64_t entry = elf_load_binary(buf, pm);
    assert(entry != 0);
    uint64_t pid = scheduler_spawn(true, (void (*)(void))entry, pm);
    trace("Spawned %s with pid %d", init_path, pid);
#if FINAL_DEBUG
    scheduler_set_final(final_debug);
#else
    scheduler_set_final(final);
#endif

    // Init the timer, aka start the scheduler
    pit_init();
    hlt();
}
