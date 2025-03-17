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

extern uint64_t kernel_stack_top;
void post_main()
{
    info("shadowOS Kernel v1.0 successfully initialized");
    assert(VFS_ROOT());
    assert(VFS_ROOT()->child);

    // vnode_t *current = VFS_ROOT()->child;
    // vnode_t *stack[256];
    // int stack_depth = 0;
    // assert(current);
    // assert(current->name);

    // while (current != NULL || stack_depth > 0)
    // {
    //     if (current != NULL)
    //     {
    //         if (current->type != VNODE_DIR)
    //             VFS_PRINT_VNODE(current);
    //         if (current->child != NULL)
    //         {
    //             stack[stack_depth++] = current->next;
    //             current = current->child;
    //         }
    //         else
    //         {
    //             current = current->next;
    //         }
    //     }
    //     else
    //     {
    //         current = stack[--stack_depth];
    //     }
    // }
    // printf("\n");

    // uint64_t free = pmm_get_free_memory();
    // uint64_t total = pmm_get_total_memory();
    // printf("Free memory:\t%llu MB\nTotal memory:\t%llu MB\n", BYTES_TO_MB(free), BYTES_TO_MB(total));
    // printf("------------------------------------------------------------\n");

    // Initialize tss
    tss_init(kernel_stack_top);

    // Finish and spawn init task, scheduler is currently broken
    scheduler_init();

    // Load init proc, in usermode
    char *bin = VFS_READ("/bin/init");
    assert(bin);
    uint64_t *pm = vmm_new_pagemap();
    trace("Loaded new pagemap at 0x%.16llx", (uint64_t)pm);
    uint64_t entry = elf_load_binary(bin, pm);
    assert(entry != 0);
    uint64_t pid = scheduler_spawn(true, (void (*)(void))entry, pm);
    (void)pid;
    trace("Spawned /bin/init with pid %d", pid);

    // Init the timer, aka start the scheduler
    pit_init();
    hlt();
}
