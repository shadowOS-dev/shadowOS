#include <fs/devfs.h>
#include <lib/log.h>
#include <lib/assert.h>
#include <lib/memory.h>
#include <mm/kmalloc.h>

mount_t *devfs_root = NULL;
vnode_ops_t devfs_ops;

typedef struct dev
{
    void (*read)(void *buf, size_t size, size_t offset);
    void (*write)(const void *buf, size_t size, size_t offset);
} dev_t;

int devfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset)
{
    (void)vnode;
    ((dev_t *)vnode->data)->read(buf, size, offset);
    return 0;
}

int devfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset)
{
    (void)vnode;
    ((dev_t *)vnode->data)->write(buf, size, offset);
    return size;
}

struct vnode *devfs_create(vnode_t *self, const char *name, vnode_type_t type)
{
    spinlock_release(&self->lock);
    if (vfs_lookup(self, name) != NULL)
    {
        error("Could not create vnode '%s' as it already exists", name);
        return NULL;
    }
    spinlock_acquire(&self->lock);

    vnode_t *new_vnode = kmalloc(sizeof(vnode_t));
    if (!new_vnode)
    {
        error("Failed to allocate memory for new vnode");
        return NULL;
    }

    strncpy(new_vnode->name, name, sizeof(new_vnode->name));
    new_vnode->type = type;
    new_vnode->child = NULL;
    new_vnode->next = NULL;

    vnode_t *current = self->child;
    if (current == NULL)
    {
        self->child = new_vnode;
    }
    else
    {
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_vnode;
    }

    new_vnode->parent = self;
    new_vnode->mount = self->mount;
    new_vnode->flags = 0;
    new_vnode->size = 0;
    new_vnode->uid = 0; // root
    new_vnode->gid = 0;
    new_vnode->creation_time = 0;
    new_vnode->access_time = 0;
    new_vnode->modify_time = 0;
    new_vnode->mode = VNODE_MODE_RUSR | VNODE_MODE_WUSR | // rw-
                      VNODE_MODE_RGRP | VNODE_MODE_WGRP;  // rw-
    new_vnode->data = NULL;
    new_vnode->ops = &devfs_ops;
    spinlock_init(&new_vnode->lock);

    trace("Created new vnode '%s' of type '%s', parent '%s'", name, (type == VNODE_DIR) ? "directory" : "file", new_vnode->parent->name);
    return new_vnode;
}

vnode_ops_t devfs_ops = {
    .read = devfs_read,
    .write = devfs_write,
    .create = devfs_create,
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

    dev->ops = &devfs_ops;
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
    devfs_dir->flags = VNODE_FLAG_MOUNTPOINT;

    mount_t *mount = vfs_mount("/dev", "devfs");
    if (!mount)
    {
        error("Failed to mount devfs at '/dev'");
        return;
    }

    devfs_root = mount;
    devfs_root->root = devfs_dir;
    devfs_dir->mount = mount;

    trace("devfs initialized at /dev");
}
