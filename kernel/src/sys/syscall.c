#include <sys/syscall.h>
#include <proc/scheduler.h>
#include <dev/vfs.h>
#include <lib/log.h>
#include <util/errno.h>
#include <lib/assert.h>
#include <dev/time/rtc.h>

// Define the syscall table with function pointers to syscalls
syscall_fn_t syscall_table[] = {
    (syscall_fn_t)sys_exit,  // SYS_exit
    (syscall_fn_t)sys_open,  // SYS_open
    (syscall_fn_t)sys_close, // SYS_close
    (syscall_fn_t)sys_write, // SYS_write
    (syscall_fn_t)sys_read,  // SYS_read
    (syscall_fn_t)sys_stat,  // SYS_stat
};

// Define the syscalls
int sys_exit(int code)
{
    s_trace("exit(%d)", code);
    scheduler_exit(code);
    return 0;
}

int sys_open(const char *path, uint64_t flags, uint8_t kind)
{
    s_trace("open(path=\"%s\", flags=%llu)", path, flags);

    vnode_t *node = vfs_lazy_lookup(VFS_ROOT()->mount, path);
    if ((flags & O_CREATE) && node == NULL) // handle create flag
    {
        node = vfs_create_vnode(vfs_lazy_lookup_last(VFS_ROOT()->mount, path), FILENAME_FROM_PATH(path), kind); // Permissions and such wont be handled by open(), go chmod it or sum idk.
    }

    node->access_time = GET_CURRENT_UNIX_TIME(); // Quick, easy, and dirty fix.

    if (node == NULL)
    {
        warning("Failed to find path \"%s\"", path);
        scheduler_get_current()->errno = ENOENT;
        return -1;
    }
    return scheduler_proc_add_vnode(scheduler_get_current()->pid, node);
}

int sys_close(int fd)
{
    s_trace("close(fd=%d)", fd);
    int s = scheduler_proc_remove_vnode(scheduler_get_current()->pid, fd);

    if (s == -1)
    {
        scheduler_get_current()->errno = EBADF;
        return -1;
    }
    else if (s == -2)
    {
        scheduler_get_current()->errno = EBADPID;
        return -1;
    }
    return 0;
}

int sys_write(int fd, void *buff, size_t size)
{
    s_trace("write(fd=%d, buff=0x%.16lx, size=%d, offset=0)", fd, (uint64_t)buff, (int)size);

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to write()");
        scheduler_get_current()->errno = EBADF;
        return -1;
    }

    if (vfs_am_i_allowed(node, scheduler_get_current()->whoami.uid, scheduler_get_current()->whoami.gid, 2) == false)
    {
        scheduler_get_current()->errno = EACCES;
        return -1;
    }

    assert(buff);
    assert(size);

    int ret = vfs_write(node, buff, size, 0);
    if (ret == -1)
    {
        scheduler_get_current()->errno = ENOTIMPL;
        return -1;
    }
    return ret;
}

int sys_read(int fd, void *buff, size_t size)
{
    s_trace("read(fd=%d, buff=0x%.16lx, size=%d, offset=0)", fd, (uint64_t)buff, (int)size);

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to read()");
        scheduler_get_current()->errno = EBADF;
        return -1;
    }

    if (vfs_am_i_allowed(node, scheduler_get_current()->whoami.uid, scheduler_get_current()->whoami.gid, 1) == false)
    {
        scheduler_get_current()->errno = EACCES;
        return -1;
    }

    assert(buff);
    assert(size);

    int ret = vfs_read(node, buff, size, 0);
    if (ret == -1)
    {
        scheduler_get_current()->errno = ENOTIMPL;
    }

    return ret;
}

int sys_stat(int fd, stat_t *stat)
{
    s_trace("stat(fd=%d, stat=0x%.16lx)", fd, (uintptr_t)stat);

    if (stat == NULL)
    {
        warning("Invalid user buffer passed to stat()");
        scheduler_get_current()->errno = EFAULT;
        return -1;
    }

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to stat()");
        scheduler_get_current()->errno = EBADF;
        return -1;
    }

    stat->flags = node->flags;
    stat->size = node->size;
    stat->type = (uint32_t)node->type;
    stat->uid = node->uid;
    stat->gid = node->gid;
    stat->mode = node->mode;
    return 0;
}
