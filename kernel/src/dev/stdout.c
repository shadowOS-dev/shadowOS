#include <dev/stdout.h>
#include <dev/vfs.h>
#include <lib/assert.h>
#include <fs/devfs.h>
#include <dev/portio.h>

vnode_t *stdout;

int read(void *, size_t, size_t)
{
    return -1;
}

extern void putchar(char);
int write(const void *buf, size_t size, size_t)
{
    assert(buf);
    for (size_t i = 0; i < size; i++)
    {
#if _GRAPHICAL_STDOUT
        putchar(*(char *)((uint8_t *)buf + i));
#endif // _GRAPHICAL_STDOUT

        // Always write to com, should be initialized (hopefully)
        outb(DEFAULT_COM_PORT, *(char *)((uint8_t *)buf + i));
    }

    return size;
}

int read_vnode(vnode_t *, void *, size_t, size_t)
{
    return 0;
}

int write_vnode(vnode_t *, const void *buf, size_t size, size_t)
{
    assert(buf);
    for (size_t i = 0; i < size; i++)
    {
        putchar(*(char *)((uint8_t *)buf + i));
    }
    return size;
}

void stdout_init()
{
    assert(devfs_add_dev("stdout", read, write) == 0);
    stdout = vfs_lazy_lookup(VFS_ROOT()->mount, "/dev/stdout");
}