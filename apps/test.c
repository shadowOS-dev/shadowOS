#include <stdint.h>

#define STDOUT 0
#define SYS_write 3
#define SYS_exit 0

static inline long syscall(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    long ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory");
    return ret;
}

void write(int fd, const char *buf, uint64_t size)
{
    syscall(SYS_write, fd, (uint64_t)buf, size);
}

void exit(int status)
{
    syscall(SYS_exit, status, 0, 0);
}

void _start(void)
{
    write(STDOUT, "Hello, World!\n", 14);
    exit(0);
}