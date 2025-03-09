#include <fs/procfs.h>
#include <fs/procfs.h>
#include <lib/log.h>
#include <lib/assert.h>
#include <lib/memory.h>
#include <mm/kmalloc.h>

typedef struct proc
{
    void *data;
    size_t size;
} proc_t;

mount_t *procfs_root = NULL;

int procfs_read(vnode_t *vnode, void *buf, size_t size, size_t offset)
{
    (void)vnode;
    proc_t *data = vnode->data;
    assert(data && offset < data->size);
    memcpy(buf, data->data + offset, size);
    return size;
}

int procfs_write(vnode_t *vnode, const void *buf, size_t size, size_t offset)
{
    (void)vnode;
    (void)buf;
    (void)size;
    (void)offset;
    error("Permission Denied: Read-only filesystem"); // still writable via manually doing it
    return 0;
}

vnode_ops_t proc_vnode_ops = {
    .read = procfs_read,
    .write = procfs_write,
};

int procfs_add_proc(const char *name, void *data, size_t size)
{
    if (name == NULL || data == NULL)
    {
        error("Invalid arguments: name or data is NULL");
        return -1;
    }

    vnode_t *proc = vfs_create_vnode(procfs_root->root, name, VNODE_FILE);
    if (!proc)
    {
        error("Failed to create process vnode for new process: '%s'", name);
        return -1;
    }

    trace("Added procfs vnode '%s', path: %s", name, vfs_get_full_path(proc));

    proc->ops = &proc_vnode_ops;
    proc_t *proc_data = kmalloc(sizeof(proc_t));
    if (!proc_data)
    {
        error("Failed to allocate memory for process data: '%s'", name);
        vfs_delete_node(proc);
        return -1;
    }

    proc_data->data = data;
    proc_data->size = size;
    proc->data = (void *)proc_data;
    proc->size = size;
    return 0;
}

void procfs_init()
{
    vnode_t *procfs_dir = vfs_create_vnode(root_mount->root, "proc", VNODE_DIR);
    assert(procfs_dir);
    procfs_dir->flags = VNODE_FLAG_MOUNTPOINT;

    mount_t *mount = vfs_mount("/proc", "procfs");
    if (!mount)
    {
        error("Failed to mount procfs at '/proc'");
        return;
    }

    procfs_root = mount;
    procfs_root->root = procfs_dir;
    procfs_dir->mount = mount;

    trace("procfs initialized at /proc");
}
