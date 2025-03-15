#include <stdint.h>
#include <stddef.h>

#define STDOUT 0
#define SYS_exit 0
#define SYS_open 1
#define SYS_close 2
#define SYS_write 3
#define SYS_read 4
#define SYS_stat 5

typedef struct stat
{
    uint64_t size;
    uint32_t flags;
    uint32_t type;
    uint32_t uid;
    uint32_t gid;
    uint32_t mode;
} stat_t;

static inline long syscall(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    long ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
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

int open(const char *pathname)
{
    return (int)syscall(SYS_open, (uint64_t)pathname, 0, 0);
}

int close(int fd)
{
    return (int)syscall(SYS_close, fd, 0, 0);
}

int read(int fd, char *buf, uint64_t size)
{
    return (int)syscall(SYS_read, fd, (uint64_t)buf, size);
}

int stat(int fd, stat_t *stat)
{
    return (int)syscall(SYS_stat, fd, (uint64_t)stat, 0);
}

void _start(void)
{
    // Print out /root/welcome.txt
    int fd = open("/root/welcome.txt");
    if (fd == -1)
    {
        exit(1);
    }

    stat_t info;
    stat(fd, &info);
    char buf[info.size];
    if (read(fd, buf, info.size) < 0)
    {
        exit(1);
    }

    write(STDOUT, buf, info.size);
    close(fd);
    exit(0);
}
