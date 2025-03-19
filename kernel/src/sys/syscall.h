#ifndef SYS_SYSCALL_H
#define SYS_SYSCALL_H

#include <stddef.h>
#include <stdint.h>
#include <dev/vfs.h>

// Define syscall IDs
#define SYS_exit 0
#define SYS_open 1
#define SYS_close 2
#define SYS_write 3
#define SYS_read 4
#define SYS_stat 5

#define SYSCALL_TABLE_SIZE 6

// Define a function pointer type for syscalls
typedef int (*syscall_fn_t)(...);

// Declare the syscall table
extern syscall_fn_t syscall_table[];

// Syscall function prototypes
int sys_exit(int code);
int sys_open(const char *path);
int sys_close(int fd);
int sys_write(int fd, void *buff, size_t size);
int sys_read(int fd, void *buff, size_t size);
int sys_stat(int fd, stat_t *stat);

#endif // SYS_SYSCALL_H
