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

void test_task()
{
    vnode_t *kbd = vfs_lazy_lookup(VFS_ROOT()->mount, "/dev/ps2kb1");
    assert(kbd);

    while (1)
    {
        uint8_t scancode;
        int read_bytes = vfs_read(kbd, &scancode, sizeof(scancode), 0);

        if (read_bytes > 0)
        {
            printf("Scancode: 0x%02X\n", scancode);
        }
    }

    scheduler_exit(0);
}

extern uint64_t kernel_stack_top;
void post_main()
{
    info("shadowOS Kernel v1.0 successfully initialized");
    assert(VFS_ROOT());
    assert(VFS_ROOT()->child);

#if _PRINT_VFS_TREE
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
#endif // _PRINT_VFS_TREE

    // Initialize tss
    tss_init(kernel_stack_top);

    // Finish and spawn init task
    scheduler_init();

    // Launch our test task
    scheduler_spawn(false, test_task, vmm_new_pagemap());

    // // Load init proc, in usermode
    // vnode_t *init = vfs_lazy_lookup(VFS_ROOT()->mount, "/bin/init");
    // if (init == NULL)
    // {
    //     error("\"/bin/init\" missing. Did you bootstrap correctly? Check your initramfs. (Halting System)");
    //     hcf();
    // }

    // char *buf = (char *)kmalloc(init->size);
    // assert(buf);
    // vfs_read(init, buf, init->size, 0);

    // VFS_READ("/bin/init");
    // uint64_t *pm = vmm_new_pagemap();
    // trace("Loaded new pagemap at 0x%.16llx", (uint64_t)pm);
    // uint64_t entry = elf_load_binary(buf, pm);
    // assert(entry != 0);
    // uint64_t pid = scheduler_spawn(true, (void (*)(void))entry, pm);
    // trace("Spawned /bin/init with pid %d", pid);

    // Init the timer, aka start the scheduler
    pit_init();
    hlt();
}
