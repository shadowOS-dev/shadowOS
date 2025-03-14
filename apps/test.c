#include <stdint.h>

#define STDOUT 0
#define SYS_write 3
#define SYS_exit 0

#define WRITE(fd, buf, size)                           \
    __asm__ volatile(                                  \
        "int $0x80"                                    \
        :                                              \
        : "a"(SYS_write), "b"(fd), "c"(buf), "d"(size) \
        : "memory")

#define EXIT(status)                 \
    __asm__ volatile(                \
        "int $0x80"                  \
        :                            \
        : "a"(SYS_exit), "b"(status) \
        : "memory")

void _start(void)
{
    WRITE(STDOUT, (char *)"Hello, World!\n", 14);
    EXIT(0);
}
