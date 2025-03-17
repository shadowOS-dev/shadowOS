#include <dev/stdout.h>
#include <dev/vfs.h>
#include <lib/assert.h>
#include <fs/devfs.h>

vnode_t *stdout;

void read(void *, size_t, size_t)
{
}

extern void putchar(char);
void write(const void *buf, size_t size, size_t)
{
    assert(buf);
    for (size_t i = 0; i < size; i++)
    {
        putchar(*(char *)((uint8_t *)buf + i));
    }
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
    // vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/"), "dev", VNODE_DIR);
    // stdout = vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/dev"), "stdout", VNODE_DEV);
    // assert(stdout);
    // stdout->ops->write = write_vnode;
    // stdout->ops->read = read_vnode;
}