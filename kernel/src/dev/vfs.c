#include <dev/vfs.h>
#include <lib/memory.h>
#include <lib/log.h>
#include <mm/kmalloc.h>

mount_t *root_mount = NULL;

void vfs_init(void)
{
    mount_t *mount = kmalloc(sizeof(mount_t));
    if (!mount)
    {
        error("Failed to allocate memory for root mount point");
        return;
    }

    mount->root = kmalloc(sizeof(vnode_t));
    if (!mount->root)
    {
        error("Failed to allocate memory for root vnode");
        kfree(mount);
        return;
    }
    strncpy(mount->root->name, "/", sizeof(mount->root->name));
    mount->root->type = VNODE_DIR;
    mount->root->child = NULL;

    mount->next = NULL;
    mount->prev = NULL;
    mount->mountpoint = "/";
    mount->type = "ramfs";
    mount->data = NULL;
    root_mount = mount;

    trace("VFS initialized with root mount point at 0x%.16llx", (uint64_t)mount);
}

vnode_t *vfs_lookup(vnode_t *parent, const char *name)
{
    vnode_t *current = parent->child;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

mount_t *vfs_mount(const char *path, const char *type)
{
    mount_t *current = root_mount;
    while (current != NULL)
    {
        if (strcmp(current->mountpoint, path) == 0)
        {
            error("Mount point '%s' is already in use", path);
            return NULL;
        }
        current = current->next;
    }

    current = root_mount;

    mount_t *new_mount = kmalloc(sizeof(mount_t));
    if (!new_mount)
    {
        error("Failed to allocate memory for mount point");
        return NULL;
    }

    new_mount->root = NULL;
    new_mount->next = root_mount;
    new_mount->prev = NULL;

    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = new_mount;
    new_mount->prev = current;
    new_mount->mountpoint = strdup(path);
    new_mount->type = strdup(type);
    new_mount->data = NULL;

    debug("Mounted '%s' with type '%s'", path, type);

    return new_mount;
}

void vfs_umount(mount_t *mount)
{
    if (!mount)
    {
        error("Trying to unmount a NULL mount point");
        return;
    }

    if (mount->prev)
    {
        mount->prev->next = mount->next;
    }
    if (mount->next)
    {
        mount->next->prev = mount->prev;
    }

    kfree(mount->mountpoint);
    kfree(mount->type);
    kfree(mount);

    debug("Unmounted a filesystem from '%s'", mount->mountpoint);
}

int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset)
{
    if (!vnode || vnode->type != VNODE_FILE)
    {
        error("Invalid vnode or not a file");
        return -1;
    }

    if (vnode->ops && vnode->ops->read)
    {
        return vnode->ops->read(vnode, buf, size, offset);
    }

    error("Read operation not implemented for vnode '%s'", vnode->name);
    return -1;
}

int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset)
{
    if (!vnode || vnode->type != VNODE_FILE)
    {
        error("Invalid vnode or not a file");
        return -1;
    }

    if (vnode->ops && vnode->ops->write)
    {
        return vnode->ops->write(vnode, buf, size, offset);
    }

    error("Write operation not implemented for vnode '%s'", vnode->name);
    return -1;
}

vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type)
{
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

    vnode_t *current = parent->child;
    if (current == NULL)
    {
        parent->child = new_vnode;
    }
    else
    {
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_vnode;
    }
    new_vnode->parent = parent;
    new_vnode->size = 0;
    new_vnode->data = NULL;
    new_vnode->ops = NULL; // You have to setup the operations based on fs type.
    trace("Created new vnode '%s' of type '%s', parent '%s'", name, (type == VNODE_DIR) ? "directory" : "file", new_vnode->parent->name);

    return new_vnode;
}

vnode_t *vfs_lazy_lookup(mount_t *mount, const char *path)
{
    if (!mount || !path || path[0] != '/')
    {
        error("Invalid mount or path");
        return NULL;
    }

    vnode_t *current_vnode = mount->root;
    if (!current_vnode)
    {
        error("No root vnode in the mount");
        return NULL;
    }

    const char *current_path = path + 1;

    char name_buffer[256];
    while (*current_path != '\0')
    {
        uint64_t i = 0;
        while (*current_path != '/' && *current_path != '\0' && i < sizeof(name_buffer) - 1)
        {
            name_buffer[i++] = *current_path++;
        }
        name_buffer[i] = '\0';

        current_vnode = vfs_lookup(current_vnode, name_buffer);
        if (!current_vnode)
        {
            // error("Path component '%s' not found", name_buffer);
            error("Invalid path '%s'", path);
            return NULL;
        }

        if (current_vnode->type == VNODE_FILE)
        {
            break;
        }

        if (*current_path == '/')
        {
            current_path++;
        }
    }

    return current_vnode;
}

char *vfs_get_full_path(vnode_t *vnode)
{
    char *full_path = kmalloc(sizeof(char) * 256);
    if (!full_path)
    {
        error("Failed to allocate memory for full path");
        return NULL;
    }

    if (vnode->parent == vnode)
    {
        strcpy(full_path, "/");
        return full_path;
    }

    char *parent_path = vfs_get_full_path(vnode->parent);
    if (!parent_path)
    {
        return NULL;
    }

    sprintf(full_path, "%s/%s", parent_path, vnode->name);
    kfree(parent_path);

    return full_path;
}