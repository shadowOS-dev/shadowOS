#include <dev/vfs.h>
#include <lib/memory.h>
#include <lib/log.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <lib/spinlock.h>

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
    mount->root->mount = mount;
    mount->root->parent = mount->root;
    mount->root->uid = 0; // root by default
    mount->root->gid = 0;
    mount->root->mode = VNODE_MODE_RUSR | VNODE_MODE_WUSR | VNODE_MODE_XUSR |
                        VNODE_MODE_RGRP | VNODE_MODE_XGRP |
                        VNODE_MODE_ROTH | VNODE_MODE_XOTH;
    spinlock_init(&mount->root->lock);

    mount->next = NULL;
    mount->prev = NULL;
    mount->mountpoint = "/";
    mount->type = "rootfs"; // will be replaced with actual filesystem type, e.g. "ramfs"
    mount->data = NULL;
    root_mount = mount;

    trace("VFS initialized with root mount point at 0x%.16llx", (uint64_t)mount);
}

vnode_t *vfs_lookup(vnode_t *parent, const char *name)
{
    spinlock_acquire(&parent->lock);
    vnode_t *current = parent->child;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            spinlock_release(&parent->lock);
            return current;
        }
        current = current->next;
    }
    spinlock_release(&parent->lock);
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
    spinlock_acquire(&vnode->lock);

    if (!vnode || vnode->type == VNODE_DIR)
    {
        error("Invalid vnode or unsupported type: %s", vfs_type_to_str(vnode->type));
        spinlock_release(&vnode->lock);
        return -1;
    }

    if (vnode->ops && vnode->ops->read)
    {
        int ret = vnode->ops->read(vnode, buf, size, offset);
        spinlock_release(&vnode->lock);
        return ret;
    }

    error("Read operation not implemented for vnode '%s'", vnode->name);
    spinlock_release(&vnode->lock);
    return -1;
}

int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset)
{
    spinlock_acquire(&vnode->lock);

    if (!vnode || vnode->type == VNODE_DIR)
    {
        error("Invalid vnode or unsupported type: %s", vfs_type_to_str(vnode->type));
        spinlock_release(&vnode->lock);
        return -1;
    }

    if (vnode->ops && vnode->ops->write)
    {
        int ret = vnode->ops->write(vnode, buf, size, offset);
        spinlock_release(&vnode->lock);
        return ret;
    }

    error("Write operation not implemented for vnode '%s'", vnode->name);
    spinlock_release(&vnode->lock);
    return -1;
}

int vfs_chown(vnode_t *vnode, uint32_t uid)
{
    spinlock_acquire(&vnode->lock);
    if (vnode == NULL)
    {
        error("Invalid vnode passed");
        spinlock_release(&vnode->lock);
        return -1;
    }

    vnode->uid = uid;
    spinlock_release(&vnode->lock);
    return 0;
}

int vfs_chmod(vnode_t *vnode, uint32_t mode)
{

    spinlock_acquire(&vnode->lock);
    if (vnode == NULL)
    {
        error("Invalid vnode passed");
        spinlock_release(&vnode->lock);
        return -1;
    }

    vnode->mode = mode;
    spinlock_release(&vnode->lock);
    return 0;
}

vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type)
{
    spinlock_acquire(&parent->lock);
    if (!parent || parent->type != VNODE_DIR)
    {
        error("Invalid parent vnode or parent is not a directory: %s", vfs_get_full_path(parent));
        spinlock_release(&parent->lock);
        return NULL;
    }

    if (parent->ops && parent->ops->create)
    {
        vnode_t *ret = parent->ops->create(parent, name, type);
        spinlock_release(&parent->lock);
        return ret;
    }

    error("Create operation not implemented for parent vnode '%s'", vfs_get_full_path(parent));
    spinlock_release(&parent->lock);
    return NULL;
}

void vfs_delete_node(vnode_t *vnode)
{
    if (!vnode)
    {
        error("Cannot delete a NULL vnode");
        return;
    }

    spinlock_acquire(&vnode->lock);

    if (vnode->type == VNODE_DIR)
    {
        vnode_t *child = vnode->child;
        while (child != NULL)
        {
            vnode_t *next_child = child->next;
            vfs_delete_node(child);
            child = next_child;
        }
    }

    trace("Deleting vnode '%s' (%s)", vnode->name, vfs_type_to_str(vnode->type));

    memset(vnode->name, 0, sizeof(vnode->name));
    vnode->type = 0;
    vnode->child = NULL;
    vnode->next = NULL;
    vnode->parent = NULL;
    vnode->size = 0;

    if (vnode->data)
    {
        kfree(vnode->data);
        vnode->data = NULL;
    }

    if (vnode->ops)
    {
        vnode->ops = NULL;
    }

    vnode->mount = NULL;
    kfree(vnode);

    spinlock_release(&vnode->lock);
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
            warning("Invalid path '%s'", path);
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

vnode_t *vfs_lazy_lookup_last(mount_t *mount, const char *path)
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
    vnode_t *last_valid_vnode = current_vnode;

    while (*current_path != '\0')
    {
        uint64_t i = 0;

        while (*current_path != '/' && *current_path != '\0' && i < sizeof(name_buffer) - 1)
        {
            name_buffer[i++] = *current_path++;
        }
        name_buffer[i] = '\0';

        vnode_t *next_vnode = vfs_lookup(current_vnode, name_buffer);
        if (!next_vnode)
        {
            trace("Invalid path component '%s', stopping lookup", name_buffer);
            return last_valid_vnode;
        }

        last_valid_vnode = next_vnode;
        current_vnode = next_vnode;

        if (*current_path == '/')
        {
            current_path++;
        }
    }

    return last_valid_vnode;
}

char *vfs_get_full_path(vnode_t *vnode)
{
    assert(vnode);
    assert(vnode->parent);
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
    printf("Mount: %s at %s\n", current_mount->type, current_mount->mountpoint);
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

        printf("%-*s%s%s (%s): %lu bytes\n", depth * 4, "", current_vnode->name, flag_str,
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

                printf("%-*s|-- %s%s (%s): %lu bytes\n", (depth + 1) * 4, "", child_vnode->name,
                       child_flag_str, vfs_type_to_str(child_vnode->type), child_vnode->size);
                child_vnode = child_vnode->next;
            }
        }

        current_vnode = current_vnode->next;
    }
}

/*
 * Actions Table:
 * 0 = Execute (check execute permission)
 * 1 = Read (check read permission)
 * 2 = Write (check write permission)
 */
bool vfs_am_i_allowed(vnode_t *vnode, uint64_t uid, uint64_t gid, uint64_t action)
{
    // Check if the vnode is NULL
    if (!vnode)
    {
        error("Invalid vnode");
        return false;
    }

    // Check if the user is the owner (uid)
    if (vnode->uid == uid)
    {
        switch (action)
        {
        case 0: // Execute
            if (vnode->mode & VNODE_MODE_XUSR)
                return true;
            break;
        case 1: // Read
            if (vnode->mode & VNODE_MODE_RUSR)
                return true;
            break;
        case 2: // Write
            if (vnode->mode & VNODE_MODE_WUSR)
                return true;
            break;
        }
    }

    // Check if the user is in the group (gid)
    if (vnode->gid == gid)
    {
        switch (action)
        {
        case 0: // Execute
            if (vnode->mode & VNODE_MODE_XGRP)
                return true;
            break;
        case 1: // Read
            if (vnode->mode & VNODE_MODE_RGRP)
                return true;
            break;
        case 2: // Write
            if (vnode->mode & VNODE_MODE_WGRP)
                return true;
            break;
        }
    }

    // If not owner and not in the group, check the other (other users) permissions
    switch (action)
    {
    case 0: // Execute
        if (vnode->mode & VNODE_MODE_XOTH)
            return true;
        break;
    case 1: // Read
        if (vnode->mode & VNODE_MODE_ROTH)
            return true;
        break;
    case 2: // Write
        if (vnode->mode & VNODE_MODE_WOTH)
            return true;
        break;
    }

    // No permission found
    return false;
}