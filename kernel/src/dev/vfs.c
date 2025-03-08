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

    vnode_t *parent_vnode = vfs_lazy_lookup(root_mount, path);
    if (!parent_vnode || parent_vnode->type != VNODE_DIR)
    {
        error("Failed to resolve path '%s' or path is not a directory", path);
        return NULL;
    }

    mount_t *new_mount = kmalloc(sizeof(mount_t));
    if (!new_mount)
    {
        error("Failed to allocate memory for mount point");
        return NULL;
    }

    new_mount->root = NULL;
    new_mount->next = NULL;
    new_mount->prev = NULL;
    new_mount->mountpoint = strdup(path);
    if (!new_mount->mountpoint)
    {
        error("Failed to allocate memory for mount point string");
        kfree(new_mount);
        return NULL;
    }

    new_mount->type = strdup(type);
    if (!new_mount->type)
    {
        error("Failed to allocate memory for mount type string");
        kfree(new_mount->mountpoint);
        kfree(new_mount);
        return NULL;
    }

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
    if (!vnode || vnode->type == VNODE_DIR)
    {
        error("Invalid vnode or unsupported type: %s", vfs_type_to_str(vnode->type));
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
    if (!vnode || vnode->type == VNODE_DIR)
    {
        error("Invalid vnode or unsupported type: %s", vfs_type_to_str(vnode->type));
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
    new_vnode->ops = NULL;
    trace("Created new vnode '%s' of type '%s', parent '%s'", name, (type == VNODE_DIR) ? "directory" : "file", new_vnode->parent->name);

    return new_vnode;
}

void vfs_delete_node(vnode_t *vnode)
{
    if (!vnode)
    {
        error("Cannot delete a NULL vnode");
        return;
    }

    memset(vnode->name, 0, sizeof(vnode->name));
    vnode->type = 0;
    vnode->child = NULL;
    vnode->next = NULL;
    vnode->parent = NULL;
    vnode->size = 0;
    vnode->data = NULL;
    vnode->ops = NULL;
    vnode->mount = NULL;

    trace("Zeroed out vnode '%s'", vnode->name);
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
            if (mount->next)
            {
                return vfs_lazy_lookup(mount->next, path);
            }
            error("Invalid path '%s'", path);
            return NULL;
        }

        if (current_vnode->type != VNODE_DIR)
        {
            break;
        }

        if (*current_path == '/')
        {
            current_path++;
        }
    }

    if (*current_path == '\0' && current_vnode->type == VNODE_DIR)
    {
        return current_vnode;
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
        full_path[0] = '\0';
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
    case VNODE_DEV:
        return "DEV";
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

    mount_t *current_mount = mount;
    while (current_mount != NULL)
    {
        debug("Mount: %s at %s", current_mount->type, current_mount->mountpoint);
        vnode_t *current_vnode = current_mount->root;
        int depth = 0;

        while (current_vnode != NULL)
        {

            char *full_path = vfs_get_full_path(current_vnode);
            assert(full_path);
            if (!full_path)
            {
                return;
            }

            char flag_str[8] = "";
            if (current_vnode->flags & VNODE_FLAG_MOUNTPOINT)
            {
                snprintf(flag_str, sizeof(flag_str), " (M)");
            }

            debug("%-*s%s%s (%s): %lu bytes", depth * 4, "", current_vnode->name, flag_str,
                  vfs_type_to_str(current_vnode->type), current_vnode->size);

            if (current_vnode->type == VNODE_DIR)
            {
                vnode_t *child_vnode = current_vnode->child;
                while (child_vnode != NULL)
                {

                    char child_flag_str[8] = "";
                    if (child_vnode->flags & VNODE_FLAG_MOUNTPOINT)
                    {
                        snprintf(child_flag_str, sizeof(child_flag_str), " (M)");
                    }

                    debug("%-*s|-- %s%s (%s): %lu bytes", (depth + 1) * 4, "", child_vnode->name, child_flag_str,
                          vfs_type_to_str(child_vnode->type), child_vnode->size);

                    if (child_vnode->type == VNODE_DIR)
                    {
                        vnode_t *sub_child_vnode = child_vnode->child;
                        while (sub_child_vnode != NULL)
                        {

                            char sub_child_flag_str[8] = "";
                            if (sub_child_vnode->flags & VNODE_FLAG_MOUNTPOINT)
                            {
                                snprintf(sub_child_flag_str, sizeof(sub_child_flag_str), " (M)");
                            }

                            debug("%-*s|-- %s%s (%s): %lu bytes", (depth + 2) * 4, "", sub_child_vnode->name, sub_child_flag_str,
                                  vfs_type_to_str(sub_child_vnode->type), sub_child_vnode->size);
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

        current_mount = current_mount->next;
    }
}
