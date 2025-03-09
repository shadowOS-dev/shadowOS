#include <lib/printf.h>
#include <dev/portio.h>
#include <stdarg.h>

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW 1
typedef long ssize_t;

#define NANOPRINTF_IMPLEMENTATION
#include <lib/nanoprintf.h>

#include <lib/flanterm/flanterm.h>
#include <dev/vfs.h>
#include <lib/memory.h>
#include <lib/log.h>

extern struct flanterm_context *ft_ctx;
extern void (*putchar_impl)(char);

size_t printk_index = 0;

extern vnode_t *stdout;

void append_to_printk_buff(const char *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        ((char *)&printk_buff_start)[printk_index] = data[i];
        printk_index = (printk_index + 1) % PRINTK_BUFF_SIZE;
    }
}

void put(const char *data, size_t length)
{
    if (!stdout)
    {
        // append to printk buffer
        append_to_printk_buff(data, length);
    }

    for (size_t i = 0; i < length; i++)
    {
        outb(0xE9, data[i]);
        if (ft_ctx)
            flanterm_write(ft_ctx, &data[i], 1);
        else if (putchar_impl)
            putchar_impl(data[i]);
    }
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (length >= 0 && length < (int)sizeof(buffer))
    {
        put(buffer, length);
    }

    va_end(args);
    return length;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = npf_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return length;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = npf_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return length;
}

int vprintf(const char *fmt, va_list args)
{
    char buffer[1024];
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (length >= 0 && length < (int)sizeof(buffer))
    {
        put(buffer, length);
    }

    return length;
}

int fprintf(vnode_t *vnode, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (length >= 0 && length < (int)sizeof(buffer))
    {
        vfs_write(vnode, buffer, length, 0);
    }

    va_end(args);
    return length;
}