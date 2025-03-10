#include <dev/stdout.h>
#include <fs/devfs.h>
#include <lib/assert.h>

vnode_t *stdout;

void read(void *buf, size_t size, size_t offset)
{
    (void)buf;
    (void)size;
    (void)offset;
    return;
}

extern void putchar(char);
void write(const void *buf, size_t size, size_t offset)
{
    (void)offset;
    assert(buf);
    for (size_t i = 0; i < size; i++)
    {
        putchar(*(char *)((uint8_t *)buf + i));
    }
}

void stdout_init()
{
    devfs_add_dev("stdout", read, write);
    stdout = vfs_lazy_lookup(VFS_ROOT()->mount, "/dev/stdout");
}