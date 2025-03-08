#ifndef FS_RAMFS_H
#define FS_RAMFS_H

#include <dev/vfs.h>

#define RAMFS_TYPE_USTAR 0x0001

void ramfs_init(mount_t *mount, int type, void *data, size_t size);

#endif // FS_RAMFS_H