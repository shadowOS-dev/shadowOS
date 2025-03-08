#include <dev/vfs.h>
#include <lib/memory.h>
#include <lib/log.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>

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

    mount_t *new_mount = kmalloc(sizeof(mount_t));
    if (!new_mount)
    {
        error("Failed to allocate memory for mount point");
        return NULL;
    }

    // Create the root vnode for the new mount
    vnode_t *root_vnode = kmalloc(sizeof(vnode_t));
    if (!root_vnode)
    {
        error("Failed to allocate memory for root vnode of mount point '%s'", path);
        kfree(new_mount);
        return NULL;
    }

    strncpy(root_vnode->name, "/", sizeof(root_vnode->name)); // Root vnode name
    root_vnode->type = VNODE_DIR;                             // Root is always a directory
    root_vnode->child = NULL;
    root_vnode->next = NULL;
    root_vnode->parent = NULL;
    root_vnode->size = 0;
    root_vnode->data = NULL;
    root_vnode->ops = NULL; // Set this according to your FS type later
    root_vnode->mount = new_mount;

    new_mount->root = root_vnode;
    new_mount->next = NULL;
    new_mount->prev = NULL;
    new_mount->mountpoint = strdup(path);
    new_mount->type = strdup(type);
    new_mount->data = NULL;

    // Start from the root mount and find the last mount point (where next is NULL)
    current = root_mount;
    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = new_mount;
    new_mount->prev = current;

    trace("Mounted '%s' with type '%s'", path, type);

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

    trace("Unmounted a filesystem from '%s'", mount->mountpoint);
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
    new_vnode->mount = parent->mount;
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
    if (vnode->parent == NULL || vnode->parent == vnode)
    {
        char *full_path = kmalloc(sizeof(char) * 2);
        if (!full_path)
        {
            error("Failed to allocate memory for full path");
            return NULL;
        }
        strcpy(full_path, "/");
        return full_path;
    }

    char *parent_path = vfs_get_full_path(vnode->parent);
    if (!parent_path)
    {
        return NULL;
    }

    size_t full_path_len = strlen(parent_path) + strlen(vnode->name) + 2;
    char *full_path = kmalloc(sizeof(char) * full_path_len);
    if (!full_path)
    {
        error("Failed to allocate memory for full path");
        kfree(parent_path);
        return NULL;
    }

    snprintf(full_path, full_path_len, "%s/%s", parent_path, vnode->name);
    kfree(parent_path);
    return full_path;
}

char *vfs_type_to_str(vnode_type_t type)
{
    switch (type)
    {
    case VNODE_FILE:
        return "FILE";
    case VNODE_DIR:
        return "DIR";
    default:
        return "UNKNOWN";
    }
}

void vfs_debug_print(mount_t *mount)
{
    if (!mount)
    {
        error("Invalid mount");
        return;
    }

    vnode_t *current_vnode = mount->root;
    int depth = 0;

    while (current_vnode != NULL)
    {
        char *full_path = vfs_get_full_path(current_vnode);
        assert(full_path);
        if (!full_path)
        {
            return;
        }

        debug("%*s%s (%s): %lu bytes", depth * 2, "", current_vnode->name, vfs_type_to_str(current_vnode->type), current_vnode->size);

        if (current_vnode->type == VNODE_DIR)
        {
            vnode_t *child_vnode = current_vnode->child;
            while (child_vnode != NULL)
            {
                debug("%*s|-- %s (%s): %lu bytes", (depth + 1) * 2, "", child_vnode->name, vfs_type_to_str(child_vnode->type), child_vnode->size);

                if (child_vnode->type == VNODE_DIR)
                {
                    vnode_t *sub_child_vnode = child_vnode->child;
                    while (sub_child_vnode != NULL)
                    {
                        debug("%*s|-- %s (%s): %lu bytes", (depth + 2) * 2, "", sub_child_vnode->name, vfs_type_to_str(sub_child_vnode->type), sub_child_vnode->size);
                        sub_child_vnode = sub_child_vnode->next;
                    }
                }

                child_vnode = child_vnode->next;
            }
        }

        kfree(full_path);
        current_vnode = current_vnode->next;
        depth++;
    }
}
