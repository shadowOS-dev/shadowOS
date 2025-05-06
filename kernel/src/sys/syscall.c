#include <sys/syscall.h>
#include <proc/scheduler.h>
#include <dev/vfs.h>
#include <lib/log.h>
#include <util/errno.h>
#include <lib/assert.h>
#include <dev/time/rtc.h>

syscall_fn_t syscall_table[] = {
    (syscall_fn_t)sys_exit,   // SYS_exit
    (syscall_fn_t)sys_open,   // SYS_open
    (syscall_fn_t)sys_close,  // SYS_close
    (syscall_fn_t)sys_write,  // SYS_write
    (syscall_fn_t)sys_read,   // SYS_read
    (syscall_fn_t)sys_stat,   // SYS_stat
    (syscall_fn_t)sys_setuid, // SYS_setuid
    (syscall_fn_t)sys_setgid, // SYS_setgid
    (syscall_fn_t)sys_ioctl,  // SYS_ioctl
    (syscall_fn_t)sys_getpid, // SYS_getpid
    (syscall_fn_t)sys_uname,  // SYS_uname
};

// Define the syscalls
int sys_exit(int code)
{
    s_trace("exit(%d)", code);
    if (!scheduler_get_current())
        return -ESRCH;
    scheduler_exit(code);
    return 0;
}

int sys_open(const char *path, uint64_t flags, uint8_t kind)
{
    s_trace("open(path=\"%s\", flags=%llu)", path, flags);
    if (!scheduler_get_current())
        return -ESRCH;

    vnode_t *node = vfs_lazy_lookup(VFS_ROOT()->mount, path);
    if ((flags & O_CREATE) && node == NULL)
    {
        node = vfs_create_vnode(vfs_lazy_lookup_last(VFS_ROOT()->mount, path), FILENAME_FROM_PATH(path), kind);
    }

    if (node == NULL)
    {
        warning("Failed to find path \"%s\"", path);
        return -ENOENT;
    }

    node->access_time = GET_CURRENT_UNIX_TIME();
    return scheduler_proc_add_vnode(scheduler_get_current()->pid, node);
}

int sys_close(int fd)
{
    s_trace("close(fd=%d)", fd);
    if (!scheduler_get_current())
        return -ESRCH;

    int s = scheduler_proc_remove_vnode(scheduler_get_current()->pid, fd);
    if (s == -1)
        return -EBADF;
    if (s == -2)
        return -EBADPID;

    return 0;
}

int sys_write(int fd, void *buff, size_t size)
{
    s_trace("write(fd=%d, buff=0x%.16lx, size=%d, offset=0)", fd, (uint64_t)buff, (int)size);
    if (!scheduler_get_current())
        return -ESRCH;

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to write()");
        return -EBADF;
    }

    if (!vfs_am_i_allowed(node, scheduler_get_current()->whoami.uid, scheduler_get_current()->whoami.gid, 2))
    {
        return -EACCES;
    }

    assert(buff);
    assert(size);

    int ret = vfs_write(node, buff, size, 0);
    return (ret == -1) ? -ENOTIMPL : ret;
}

int sys_read(int fd, void *buff, size_t size)
{
    s_trace("read(fd=%d, buff=0x%.16lx, size=%d, offset=0)", fd, (uint64_t)buff, (int)size);
    if (!scheduler_get_current())
        return -ESRCH;

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to read()");
        return -EBADF;
    }

    if (!vfs_am_i_allowed(node, scheduler_get_current()->whoami.uid, scheduler_get_current()->whoami.gid, 1))
    {
        return -EACCES;
    }

    assert(buff);
    assert(size);

    int ret = vfs_read(node, buff, size, 0);
    return (ret == -1) ? -ENOTIMPL : ret;
}

int sys_stat(int fd, stat_t *stat)
{
    s_trace("stat(fd=%d, stat=0x%.16lx)", fd, (uintptr_t)stat);
    if (!scheduler_get_current())
        return -ESRCH;

    if (stat == NULL)
    {
        warning("Invalid user buffer passed to stat()");
        return -EFAULT;
    }

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to stat()");
        return -EBADF;
    }

    stat->flags = node->flags;
    stat->size = node->size;
    stat->type = (uint32_t)node->type;
    stat->uid = node->uid;
    stat->gid = node->gid;
    stat->mode = node->mode;
    return 0;
}

int sys_setuid(uint32_t uid)
{
    s_trace("setuid(uid=%d)", uid);
    if (!scheduler_get_current())
        return -ESRCH;
    scheduler_get_current()->whoami.uid = uid;
    return 0;
}

int sys_setgid(uint32_t gid)
{
    s_trace("setgid(gid=%d)", gid);
    if (!scheduler_get_current())
        return -ESRCH;
    scheduler_get_current()->whoami.gid = gid;
    return 0;
}

int sys_ioctl(int fd, uint32_t cmd, uint32_t arg)
{
    s_trace("ioctl(fd=%d, cmd=0x%x, arg=0x%x)", fd, cmd, arg);
    if (!scheduler_get_current())
        return -ESRCH;

    vnode_t *node = scheduler_get_current()->fd_table[fd];
    if (node == NULL)
    {
        warning("Invalid file descriptor passed to ioctl()");
        return -EBADF;
    }

    if (node->ops->ioctl == NULL)
    {
        return -ENOTTY;
    }

    return node->ops->ioctl(node, cmd, arg);
}

int sys_getpid()
{
    s_trace("getpid()");
    if (!scheduler_get_current())
        return -ESRCH;

    return scheduler_get_current()->pid;
}

int sys_uname(uname_t *buf)
{
    return -ENOTIMPL;
    if (!buf)
        return -EINVAL;

    memset(buf, 0, sizeof(uname_t));

    snprintf(buf->sysname, sizeof(buf->sysname), "Shadow");
    snprintf(buf->nodename, sizeof(buf->nodename), "localhost");
    snprintf(buf->release, sizeof(buf->release), "1.0.0-alpha");
    snprintf(buf->machine, sizeof(buf->machine), "x86_64");
    snprintf(buf->os_type, sizeof(buf->os_type), "shadowOS");

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

    snprintf(buf->build, sizeof(buf->build), "#1 %s", GIT_HASH);
    snprintf(buf->version, sizeof(buf->version), "Shadow v1.0.0-alpha");

    return 0;
}
