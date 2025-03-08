#ifndef DEV_VFS_H
#define DEV_VFS_H

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    VNODE_DIR,
    VNODE_FILE
} vnode_type_t;

struct vnode;
struct mount;

typedef struct vnode_ops
{
    int (*read)(struct vnode *vnode, void *buf, size_t size, size_t offset);
    int (*write)(struct vnode *vnode, const void *buf, size_t size, size_t offset);
} vnode_ops_t;

typedef struct vnode
{
    struct vnode *parent;
    struct vnode *next;
    struct vnode *child;
    struct mount *mount;

    vnode_type_t type;
    char name[256];
    uint64_t size;
    void *data;

    vnode_ops_t *ops;

} vnode_t;

// TODO: Support mounts in mounts
typedef struct mount
{
    vnode_t *root;      // Root vnode of the filesystem
    struct mount *next; // Next mount point
    struct mount *prev; // Previous mount point
    char *mountpoint;   // Path to the mount point, e.g. /mnt/
    char *type;         // File system type, e.g. "ramfs"
    void *data;         // Private data for the file system
} mount_t;

extern mount_t *root_mount;

void vfs_init(void);
vnode_t *vfs_lookup(vnode_t *parent, const char *name);
mount_t *vfs_mount(const char *path, const char *type);
vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type);
void vfs_umount(mount_t *mount);
int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset);
int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset);
vnode_t *vfs_lazy_lookup(mount_t *mount, const char *path);
char *vfs_get_full_path(vnode_t *vnode);
void vfs_debug_print(mount_t *mount);

#define VFS_ROOT() (root_mount->root)

#endif // DEV_VFS_H
