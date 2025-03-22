#ifndef SYS_SYSCALL_H
#define SYS_SYSCALL_H

#include <stddef.h>
#include <stdint.h>
#include <dev/vfs.h>

// open() flags--
#define O_CREATE BIT(0)
// --end

// Define syscall IDs
#define SYS_exit 0
#define SYS_open 1
#define SYS_close 2
#define SYS_write 3
#define SYS_read 4
#define SYS_stat 5
#define SYS_setuid 6
#define SYS_setgid 7
#define SYS_ioctl 8
#define SYS_getpid 9
#define SYS_uname 10

#define SYSCALL_TABLE_SIZE 11

typedef struct
{
    char sysname[64];
    char nodename[64];
    char release[64];
    char version[128];
    char machine[64];
    char build[32];
    char os_type[64];
} uname_t;

typedef int (*syscall_fn_t)(...);
extern syscall_fn_t syscall_table[];

int sys_exit(int code);
int sys_open(const char *path, uint64_t flags, uint8_t kind);
int sys_close(int fd);
int sys_write(int fd, void *buff, size_t size);
int sys_read(int fd, void *buff, size_t size);
int sys_stat(int fd, stat_t *stat);
int sys_setuid(uint32_t uid);
int sys_setgid(uint32_t gid);
int sys_ioctl(int fd, uint32_t cmd, uint32_t arg);
int sys_getpid();
int sys_uname(uname_t *buf);

#define SYSCALL_TO_STR(number)                                       \
    ((number) == SYS_exit ? "exit" : (number) == SYS_open ? "open"   \
                                 : (number) == SYS_close  ? "close"  \
                                 : (number) == SYS_write  ? "write"  \
                                 : (number) == SYS_read   ? "read"   \
                                 : (number) == SYS_stat   ? "stat"   \
                                 : (number) == SYS_setuid ? "setuid" \
                                 : (number) == SYS_setgid ? "setgid" \
                                 : (number) == SYS_ioctl  ? "ioctl"  \
                                 : (number) == SYS_getpid ? "getpid" \
                                 : (number) == SYS_uname  ? "uname"  \
                                                          : "unknown")

static inline long
syscall(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    long ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
        : "memory");
    return ret;
}

#endif // SYS_SYSCALL_H
