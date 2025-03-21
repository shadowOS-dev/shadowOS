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

#define SYSCALL_TABLE_SIZE 9

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

#endif // SYS_SYSCALL_H
