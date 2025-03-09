#ifndef FS_RAMFS_H
#define FS_RAMFS_H

#include <dev/vfs.h>

#define RAMFS_TYPE_USTAR 0x0001

typedef struct ramfs_data
{
    void *data;
    size_t size;
} ramfs_data_t;

extern vnode_ops_t ramfs_ops;
void ramfs_init(mount_t *mount, int type, void *data, size_t size);

#endif // FS_RAMFS_H