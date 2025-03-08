#include <fs/devfs.h>
#include <lib/log.h>
#include <lib/assert.h>
#include <lib/memory.h>
#include <mm/kmalloc.h>

mount_t *devfs_root = NULL;

typedef struct dev
{
    void (*read)(void *buf, size_t size, size_t offset);
    void (*write)(const void *buf, size_t size, size_t offset);
} dev_t;

int devfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset)
{
    debug("devfs recived read request for device: '%s'", vfs_get_full_path(vnode));
    (void)vnode;
    ((dev_t *)vnode->data)->read(buf, size, offset);
    return 0;
}

int devfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset)
{
    debug("devfs recived write request for device: '%s'", vfs_get_full_path(vnode));
    (void)vnode;
    ((dev_t *)vnode->data)->write(buf, size, offset);
    return size;
}

vnode_ops_t dev_vnode_ops = {
    .read = devfs_read,
    .write = devfs_write,
};

int devfs_add_dev(const char *name, void (*read)(void *, size_t, size_t), void (*write)(const void *, size_t, size_t))
{
    if (name == NULL || read == NULL || write == NULL)
    {
        error("Invalid arguments: name, read, or write function is NULL");
        return -1;
    }

    vnode_t *dev = vfs_create_vnode(devfs_root->root, name, VNODE_DEV);
    if (!dev)
    {
        error("Failed to create device vnode for new device: '%s'", name);
        return -1;
    }
    trace("Added device vnode '%s', path: %s", name, vfs_get_full_path(dev));

    dev->ops = &dev_vnode_ops;
    dev_t *device = kmalloc(sizeof(dev_t));
    if (!device)
    {
        error("Failed to allocate memory for device data: '%s'", name);
        vfs_delete_node(dev);
        return -1;
    }

    device->read = read;
    device->write = write;
    dev->data = (void *)device;

    trace("Device '%s' successfully added to devfs", name);
    return 0;
}

void devfs_init()
{
    vnode_t *devfs_dir = vfs_create_vnode(root_mount->root, "dev", VNODE_DIR);
    assert(devfs_dir);

    mount_t *mount = vfs_mount("/dev", "devfs");
    if (!mount)
    {
        error("Failed to mount devfs at '/dev'");
        return;
    }

    devfs_root = mount;
    devfs_root->root = devfs_dir;

    trace("devfs initialized at /dev");
}
