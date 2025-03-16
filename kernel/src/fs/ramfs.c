#include <fs/ramfs.h>
#include <lib/log.h>
#include <lib/assert.h>
#include <lib/memory.h>
#include <stdbool.h>
#include <mm/kmalloc.h>

#define USTAR_HEADER_SIZE 512
#define NAME_SIZE 100

typedef struct ustar_header
{
    char name[NAME_SIZE];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[NAME_SIZE];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} ustar_header_t;

int ramfs_read(struct vnode *vnode, void *buf, size_t size, size_t offset)
{
    trace("ramfs_read(vnode=%p, buf=%p, size=%zu, offset=%zu)", vnode, buf, size, offset);

    if (!vnode || vnode->type != VNODE_FILE)
    {
        error("Invalid vnode or not a file");
        return -1;
    }

    ramfs_data_t *data = vnode->data;
    if (!data || offset >= data->size)
    {
        error("Invalid offset for read operation, data: %s", data ? "ok" : "null");
        return 0;
    }

    size_t to_read = size > (data->size - offset) ? (data->size - offset) : size;
    if (!buf)
    {
        error("Buffer is NULL");
        return -1;
    }

    memcpy(buf, data->data + offset, to_read);
    return to_read;
}

int ramfs_write(struct vnode *vnode, const void *buf, size_t size, size_t offset)
{
    trace("ramfs_write(vnode=%p, buf=%p, size=%zu, offset=%zu)", vnode, buf, size, offset);

    if (!vnode || vnode->type != VNODE_FILE)
    {
        error("Invalid vnode or not a file");
        return -1;
    }

    ramfs_data_t *data = vnode->data;
    if (!data)
    {
        error("Invalid data for write operation");
        return -1;
    }

    if (offset >= data->size)
    {
        size_t new_size = offset + size;
        if (new_size > data->size)
        {
            void *new_data = kmalloc(new_size);
            if (!new_data)
            {
                error("Failed to allocate memory for expanding the file data");
                return -1;
            }

            memcpy(new_data, data->data, data->size);
            kfree(data->data);
            data->data = new_data;
            data->size = new_size;
            vnode->size = new_size;
            trace("Resized file data buffer to %zu bytes", new_size);
        }
    }

    if (!buf)
    {
        error("Buffer is NULL");
        return -1;
    }

    memcpy(data->data + offset, buf, size);
    vnode->size = (vnode->size > offset + size) ? vnode->size : (offset + size);
    data->size = vnode->size;
    return size;
}

struct vnode *ramfs_create(vnode_t *self, const char *name, vnode_type_t type)
{
    spinlock_release(&self->lock);

    if (!name || vfs_lookup(self, name) != NULL)
    {
        error("Could not create vnode '%s' as it already exists or invalid name", name);
        return NULL;
    }

    spinlock_acquire(&self->lock);

    vnode_t *new_vnode = (vnode_t *)kmalloc(sizeof(vnode_t));
    if (!new_vnode)
    {
        error("Failed to allocate memory for new vnode");
        return NULL;
    }

    strncpy(new_vnode->name, name, sizeof(new_vnode->name) - 1);
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

    if (type != VNODE_DEV)
    {
        new_vnode->mode = VNODE_MODE_RUSR | VNODE_MODE_WUSR | // rw-
                          VNODE_MODE_RGRP |                   // r--
                          VNODE_MODE_ROTH;                    // r--
    }
    else
    {
        new_vnode->mode = VNODE_MODE_RUSR | VNODE_MODE_WUSR | // rw-
                          VNODE_MODE_RGRP;                    // r--
    }

    ramfs_data_t *data = kmalloc(sizeof(ramfs_data_t));
    if (!data)
    {
        error("Failed to allocate memory for ramfs data");
        kfree(new_vnode);
        return NULL;
    }

    data->data = NULL;
    data->size = 0;
    new_vnode->data = data;
    new_vnode->ops = &ramfs_ops;
    spinlock_init(&new_vnode->lock);

    trace("Created new vnode '%s' of type '%s', parent '%s'", name, (type == VNODE_DIR) ? "directory" : "file", new_vnode->parent->name);
    return new_vnode;
}

vnode_ops_t ramfs_ops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .create = ramfs_create,
};

void ramfs_init_ustar(mount_t *mount, void *data, size_t size)
{
    assert(mount);
    assert(data);
    assert(size >= USTAR_HEADER_SIZE);

    size_t offset = 0;
    while (offset < size)
    {
        ustar_header_t *header = (ustar_header_t *)(data + offset);
        if (header->name[0] == '\0')
        {
            trace("Reached end of USTAR archive");
            break;
        }

        uint32_t file_size = strtol(header->size, NULL, 8);
        bool dir = header->typeflag == '5';
        char *name = header->name;
        if (strncmp(header->name, "./", 2) == 0)
        {
            name += 2;
        }

        if (strlen(name) == 0 || strcmp(name, ".") == 0)
        {
            trace("Skipping entry with empty name or current directory '.'");
            offset += USTAR_HEADER_SIZE + ((file_size + 511) & ~511);
            continue;
        }

        trace("Found entry: name=%s, size=%u, mode=%o, dir=%d", name, file_size, strtol(header->mode, NULL, 8), dir);
        char *token = strtok(name, "/");
        struct vnode *cur_parent = mount->root;
        while (token != NULL)
        {
            struct vnode *subdir = vfs_lookup(cur_parent, token);
            if (!subdir)
            {
                if (dir)
                {
                    vnode_t *node = vfs_create_vnode(cur_parent, token, VNODE_DIR);
                    if (node == NULL)
                    {
                        error("Failed to create directory '%s'", token);
                        return;
                    }

                    node->ops = &ramfs_ops;
                    node->mode = strtol(header->mode, NULL, 8);
                    node->creation_time = strtol(header->mtime, NULL, 8);
                }
                else
                {
                    break;
                }
            }

            cur_parent = subdir;
            token = strtok(NULL, "/");
        }

        if (!dir)
        {
            char *filename = strtok(token, "/");
            if (filename && strlen(filename) > 0)
            {
                struct vnode *file = vfs_create_vnode(cur_parent, filename, VNODE_FILE);
                if (file == NULL)
                {
                    error("Failed to create file '%s'", filename);
                    return;
                }

                ramfs_data_t *ramfs_data = kmalloc(sizeof(ramfs_data_t));
                if (!ramfs_data)
                {
                    error("Failed to allocate memory for ramfs data");
                    return;
                }

                ramfs_data->data = kmalloc(file_size);
                if (!ramfs_data->data)
                {
                    error("Failed to allocate memory for file data");
                    kfree(ramfs_data);
                    return;
                }

                memcpy(ramfs_data->data, (uint8_t *)header + USTAR_HEADER_SIZE, file_size);
                ramfs_data->size = file_size;

                file->ops = &ramfs_ops;
                file->data = (void *)ramfs_data;
                file->size = file_size;
                file->mode = strtol(header->mode, NULL, 8);
                file->creation_time = strtol(header->mtime, NULL, 8);
            }
        }
        offset += USTAR_HEADER_SIZE + ((file_size + 511) & ~511);
    }
}

void ramfs_init(mount_t *mount, int type, void *data, size_t size)
{
    assert(mount);

    // Overwrite the existing root ops with ramfs_ops, and set the type to "ramfs".
    mount->root->ops = &ramfs_ops;
    mount->type = "ramfs";
    switch (type)
    {
    case RAMFS_TYPE_USTAR:
        ramfs_init_ustar(mount, data, size);
        break;
    default:
        error("Unsupported ramfs type: %d", type);
        return;
    }

    assert(mount);
}
