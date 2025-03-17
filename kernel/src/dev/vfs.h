#ifndef DEV_VFS_H
#define DEV_VFS_H

#include <stdint.h>
#include <stddef.h>
#include <lib/spinlock.h>
#include <lib/memory.h>
#include <stdbool.h>

/* Vnode type definitions */
typedef enum
{
    VNODE_DIR = 0x0001,
    VNODE_FILE = 0x0002,
    VNODE_DEV = 0x0003,
} vnode_type_t;

/* Flags and permission modes */
#define VNODE_FLAG_MOUNTPOINT 0x0001

#define VNODE_MODE_RUSR 0x0100 // Read permission for the owner
#define VNODE_MODE_WUSR 0x0080 // Write permission for the owner
#define VNODE_MODE_XUSR 0x0040 // Execute permission for the owner

#define VNODE_MODE_RGRP 0x0020 // Read permission for the group
#define VNODE_MODE_WGRP 0x0010 // Write permission for the group
#define VNODE_MODE_XGRP 0x0008 // Execute permission for the group

#define VNODE_MODE_ROTH 0x0004 // Read permission for others
#define VNODE_MODE_WOTH 0x0002 // Write permission for others
#define VNODE_MODE_XOTH 0x0001 // Execute permission for others

struct vnode;
struct mount;

/* Operations on vnodes */
typedef struct vnode_ops
{
    int (*read)(struct vnode *vnode, void *buf, size_t size, size_t offset);
    int (*write)(struct vnode *vnode, const void *buf, size_t size, size_t offset);
    struct vnode *(*create)(struct vnode *self, const char *name, vnode_type_t type);
} vnode_ops_t;

/* Vnode structure. */
typedef struct vnode
{
    struct vnode *parent;
    struct vnode *next;
    struct vnode *child;
    struct mount *mount;

    vnode_type_t type;
    char *name;

    uint64_t size;
    void *data;

    uint32_t uid;
    uint32_t gid;

    uint32_t mode; // Permissions

    // Unix time format timestamps
    uint32_t creation_time;
    uint32_t access_time;
    uint32_t modify_time;

    vnode_ops_t *ops;
    uint32_t flags;

    spinlock_t lock;
} vnode_t;

/* Mount structure */
typedef struct mount
{
    vnode_t *root;
    struct mount *next;
    struct mount *prev;
    char *mountpoint;
    char *type;
    void *data;
} mount_t;

/* For syscalls */
typedef struct stat
{
    uint64_t size;
    uint32_t flags;
    uint32_t type;
    uint32_t uid;
    uint32_t gid;
    uint32_t mode;
} stat_t;

extern mount_t *root_mount;

void vfs_init(void);
vnode_t *vfs_lookup(vnode_t *parent, const char *name);
mount_t *vfs_mount(const char *path, const char *type);
vnode_t *vfs_create_vnode(vnode_t *parent, const char *name, vnode_type_t type);
void vfs_umount(mount_t *mount);
int vfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset);
int vfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset);
int vfs_chown(vnode_t *vnode, uint32_t uid);
int vfs_chmod(vnode_t *vnode, uint32_t mode);
vnode_t *vfs_lazy_lookup(mount_t *mount, const char *path);
vnode_t *vfs_lazy_lookup_last(mount_t *mount, const char *path);
char *vfs_get_full_path(vnode_t *vnode);
void vfs_debug_print(mount_t *mount);
char *vfs_type_to_str(vnode_type_t type);
void vfs_delete_node(vnode_t *vnode);
bool vfs_am_i_allowed(vnode_t *vnode, uint64_t uid, uint64_t gid, uint64_t action);

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

#define TIMESTAMP_TO_STRING(timestamp, utc_offset) ({ \
    static char buffer[20]; \
    uint64_t t = (timestamp) + (utc_offset) * 3600ULL; \
    int y = 1970, m = 0, d = 1, h, min, s, days; \
    const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; \
    while (t >= ((y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365) * 86400ULL) \
        t -= ((y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365) * 86400ULL, y++; \
    for (m = 0; m < 12; m++) { \
        days = dim[m] + (m == 1 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))); \
        if (t < days * 86400ULL) break; \
        t -= days * 86400ULL; \
    } \
    d += t / 86400; t %= 86400; h = t / 3600; t %= 3600; min = t / 60; s = t % 60; \
    buffer[0] = '0' + (y / 1000) % 10; buffer[1] = '0' + (y / 100) % 10; \
    buffer[2] = '0' + (y / 10) % 10; buffer[3] = '0' + (y % 10); buffer[4] = '-'; \
    buffer[5] = '0' + (m + 1) / 10; buffer[6] = '0' + (m + 1) % 10; buffer[7] = '-'; \
    buffer[8] = '0' + d / 10; buffer[9] = '0' + d % 10; buffer[10] = ' '; \
    buffer[11] = '0' + h / 10; buffer[12] = '0' + h % 10; buffer[13] = ':'; \
    buffer[14] = '0' + min / 10; buffer[15] = '0' + min % 10; buffer[16] = ':'; \
    buffer[17] = '0' + s / 10; buffer[18] = '0' + s % 10; buffer[19] = 0; buffer; })

#define VFS_PRINT_VNODE(node)                                                                   \
    {                                                                                           \
        char perms[10] = "---------";                                                           \
        perms[0] = (node->mode & VNODE_MODE_RUSR) ? 'r' : '-';                                  \
        perms[1] = (node->mode & VNODE_MODE_WUSR) ? 'w' : '-';                                  \
        perms[2] = (node->mode & VNODE_MODE_XUSR) ? 'x' : '-';                                  \
        perms[3] = (node->mode & VNODE_MODE_RGRP) ? 'r' : '-';                                  \
        perms[4] = (node->mode & VNODE_MODE_WGRP) ? 'w' : '-';                                  \
        perms[5] = (node->mode & VNODE_MODE_XGRP) ? 'x' : '-';                                  \
        perms[6] = (node->mode & VNODE_MODE_ROTH) ? 'r' : '-';                                  \
        perms[7] = (node->mode & VNODE_MODE_WOTH) ? 'w' : '-';                                  \
        perms[8] = (node->mode & VNODE_MODE_XOTH) ? 'x' : '-';                                  \
        perms[9] = '\0';                                                                        \
        printf("path=%-38s size=%-5llu type=%-4s flags=%s uid=%d gid=%d perms=%s created=%s\n", \
               vfs_get_full_path(node), node->size,                                             \
               vfs_type_to_str(node->type),                                                     \
               (node->flags & VNODE_FLAG_MOUNTPOINT) ? "(M)" : "(-)",                           \
               node->uid, node->gid, perms, TIMESTAMP_TO_STRING(node->creation_time, 0));       \
    }

#endif // DEV_VFS_H
