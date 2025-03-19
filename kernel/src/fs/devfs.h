#ifndef FS_DEVFS_H
#define FS_DEVFS_H

#include <dev/vfs.h>

#define TTY_WRITE(dev, buf) vfs_write(dev, buf, strlen(buf), 0)

void devfs_init();
int devfs_add_dev(const char *name, int (*read)(void *, size_t, size_t), int (*write)(const void *, size_t, size_t));

#endif // FS_DEVFS_H