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

void idle()
{
    while (1)
        ;
}

void user_test()
{
    // outb is protected (can't call it on usermode)
    // uint8_t value = 'A';
    // uint16_t port = 0xE9;
    // __asm__ volatile("outb %0, %1" ::"a"(value), "Nd"(port) : "memory");
    while (1) {
        __asm__ volatile ("pause");
    }
}

extern uint64_t kernel_stack_top;
void post_main()
{
    trace("hello from post_main");
    // assert(VFS_ROOT());
    // assert(VFS_ROOT()->child);

    // // print the root filesystem
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

    // // print out free memory
    // uint64_t free = pmm_get_free_memory();
    // uint64_t total = pmm_get_total_memory();
    // printf("Free memory:\t%llu MB\nTotal memory:\t%llu MB\n", BYTES_TO_MB(free), BYTES_TO_MB(total));
    // printf("------------------------------------------------------------\n");

    // Enter usermode
    tss_init(kernel_stack_top);

    // Allocate 16KiB (4 pages) for our test function
    for (int i = 0; i < 4; i++)
    {
        vmm_map(kernel_pagemap, 0x80000 + (PAGE_SIZE * i), (uint64_t)pmm_request_page(), VMM_PRESENT | VMM_WRITE | VMM_USER);
    }

    // Copy the function to 0x80000
    memset((void *)0x80000, 0, PAGE_SIZE * 4);
    memcpy((void *)0x80000, user_test, 0xc);

    vmm_map(kernel_pagemap, 0x70000, (uint64_t)pmm_request_page(), VMM_PRESENT | VMM_WRITE | VMM_USER);
    memset((void*)0x70000, 0, PAGE_SIZE);

    // Jump to the func
    jump_user(0x80000, 0x70000);

    // Finish and spawn init task
    info("shadowOS Kernel v1.0 successfully initialized");

    scheduler_init();
    scheduler_spawn(idle, kernel_pagemap);

    // Load init proc
    char *bin = VFS_READ("/bin/init");
    assert(bin);
    uint64_t *pm = vmm_new_pagemap();
    trace("Loaded new pagemap at 0x%.16llx", (uint64_t)pm);
    uint64_t entry = elf_load_binary(bin, pm);
    assert(entry != 0);
    uint64_t pid = scheduler_spawn((void (*)(void))entry, pm);

    // Try to set the user of the init proc to kevin
    scheduler_proc_change_whoami(pid, 1000);

    // Init the timer, aka start the scheduler
    pit_init();
    hlt();
}
