#include <dev/vfs.h>
#include <lib/printf.h>
#include <dev/stdout.h>
#include <mm/pmm.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <dev/timer/pit.h>

void post_main()
{
    // Print the first 70 bytes of the boot log
    vnode_t *boot_log = vfs_lazy_lookup(VFS_ROOT()->mount, "/var/log/boot.log");
    assert(boot_log);
    char *buf = kmalloc(boot_log->size);
    vfs_read(boot_log, buf, boot_log->size, 0);
    assert(buf);
    fwrite(stdout, buf, 70);
    kfree(buf);
    printf("\n");

    // we done
    char *uptime = VFS_READ("/proc/uptime");
    char *cpuinfo = VFS_READ("/proc/cpuinfo");
    printf("uptime: %s\n", uptime);
    printf("%s", cpuinfo);
    printf("\n");
    kfree(uptime);
    kfree(cpuinfo);

    // print the root filesystem
    vnode_t *current = VFS_ROOT()->child;
    vnode_t *stack[256];
    int stack_depth = 0;

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

    // print out free memory
    uint64_t free = pmm_get_free_memory();
    uint64_t total = pmm_get_total_memory();
    printf("Free memory: %llu MB\nTotal memory: %llu MB\n", BYTES_TO_MB(free), BYTES_TO_MB(total));
}
