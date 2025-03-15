#ifndef DEV_VFS_H
#define DEV_VFS_H

#include <stdint.h>
#include <stddef.h>
#include <lib/spinlock.h>
#include <lib/memory.h>

typedef enum
{
    VNODE_DIR = 0x0001,
    VNODE_FILE = 0x0002,
    VNODE_DEV = 0x0003,
} vnode_type_t;

#define VNODE_FLAG_MOUNTPOINT 0x0001

struct vnode;
struct mount;

typedef struct vnode_ops
{
    int (*read)(struct vnode *vnode, void *buf, size_t size, size_t offset);
    int (*write)(struct vnode *vnode, const void *buf, size_t size, size_t offset);
    struct vnode *(*create)(struct vnode *self, const char *name, vnode_type_t type);
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
    uint32_t flags;

    spinlock_t lock;
} vnode_t;

typedef struct mount
{
    vnode_t *root;
    struct mount *next;
    struct mount *prev;
    char *mountpoint;
    char *type;
    void *data;
} mount_t;

// For use in only syscalls, todo: move
typedef struct stat
{
    uint64_t size;
    uint32_t flags;
    uint32_t type;
} stat_t;

extern mount_t *root_mount;

void vfs_init(void);
vnode_t *vfs_lookup(vnode_t *parent, const char *name);
mount_t *vfs_mount(const char *path, const char *type);
vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type);
void vfs_umount(mount_t *mount);
int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset);
int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset);
vnode_t *vfs_lazy_lookup(mount_t *mount, const char *path);
vnode_t *vfs_lazy_lookup_last(mount_t *mount, const char *path);
char *vfs_get_full_path(vnode_t *vnode);
void vfs_debug_print(mount_t *mount);
char *vfs_type_to_str(vnode_type_t type);
void vfs_delete_node(vnode_t *vnode);

#define VFS_ROOT() (root_mount->root)
#define VFS_GET(path) (vfs_lazy_lookup(root_mount, path))
#define VFS_EXISTS(path) (VFS_GET(path) != NULL)
#define VFS_GET_LAST_IN_PATH(path)                                                      \
    ({                                                                                  \
        const char *last = strrchr((path), '/');                                        \
        last ? (*(last + 1) ? last + 1 : (last == (path) ? "/" : (last - 1))) : (path); \
    })
#define VFS_CREATE(path, type)                                           \
    ({                                                                   \
        vnode_t *parent = vfs_lazy_lookup_last(VFS_ROOT()->mount, path); \
        assert(parent);                                                  \
        vfs_create_vnode(parent, VFS_GET_LAST_IN_PATH(path), type);      \
    })
#define VFS_READ(path)                       \
    ({                                       \
        vnode_t *node = VFS_GET(path);       \
        assert(node);                        \
        char *buf = kmalloc(node->size + 1); \
        buf[node->size] = 0;                 \
        vfs_read(node, buf, node->size, 0);  \
        buf;                                 \
    })
#define VFS_WRITE(path, buf, size)     \
    ({                                 \
        vnode_t *node = VFS_GET(path); \
        assert(node);                  \
        vfs_write(node, buf, size, 0); \
    })
#define VFS_PRINT_VNODE(node)                               \
    printf("path=%-40s size=%-10llu type=%-10s flags=%s\n", \
           vfs_get_full_path(node), node->size,             \
           vfs_type_to_str(node->type),                     \
           (node->flags & VNODE_FLAG_MOUNTPOINT) ? "(M)" : "(-)")

#endif // DEV_VFS_H
