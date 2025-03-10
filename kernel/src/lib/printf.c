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
#include <lib/spinlock.h>

extern struct flanterm_context *ft_ctx;
extern void (*putchar_impl)(char);
extern vnode_t *stdout;

size_t printk_index = 0;
spinlock_t printk_lock = SPINLOCK_INIT;

void append_to_printk_buff(const char *data, size_t length)
{
    spinlock_acquire(&printk_lock);
    for (size_t i = 0; i < length; i++)
    {
        ((char *)&printk_buff_start)[printk_index] = data[i];
        printk_index = (printk_index + 1) % PRINTK_BUFF_SIZE;
    }
    spinlock_release(&printk_lock);
}

void put(const char *data, size_t length)
{
    if (stdout)
    {
        return;
    }

    append_to_printk_buff(data, length);

    for (size_t i = 0; i < length; i++)
    {
        spinlock_acquire(&printk_lock);
        outb(0xE9, data[i]);
        if (ft_ctx)
            flanterm_write(ft_ctx, &data[i], 1);
        else if (putchar_impl)
            putchar_impl(data[i]);
        spinlock_release(&printk_lock);
    }
}

int fwrite(vnode_t *vnode, const void *buffer, size_t size)
{
    size_t total_written = 0;
    while (total_written < size)
    {
        ssize_t written = vfs_write(vnode, (const char *)buffer + total_written, size - total_written, 0);
        if (written <= 0)
        {
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

int kprintf(const char *fmt, ...)
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

int vfprintf(vnode_t *vnode, const char *fmt, va_list args)
{
    if (vnode == NULL)
    {
        return 0;
    }

    char buffer[1024];
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (length >= 0 && length < (int)sizeof(buffer))
    {
        vfs_write(vnode, buffer, length, 0);
    }

    return length;
}

int fprintf(vnode_t *vnode, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vfprintf(vnode, fmt, args);
    va_end(args);
    return length;
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vfprintf(stdout, fmt, args);
    va_end(args);
    return length;
}
