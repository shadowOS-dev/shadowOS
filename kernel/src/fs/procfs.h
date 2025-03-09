#ifndef FS_PROCFS_H
#define FS_PROCFS_H

#include <dev/vfs.h>

int procfs_add_proc(const char *name, void *data, size_t size);
void procfs_init();

#endif // FS_PROCFS_H