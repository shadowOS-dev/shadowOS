#ifndef LIB_PRINTF_H
#define LIB_PRINTF_H

#include <stddef.h>
#include <stdarg.h>
#include <dev/vfs.h>

extern char printk_buff_start;
extern char printk_buff_end;

#define PRINTK_BUFF_SIZE ((size_t)&printk_buff_end - (size_t)&printk_buff_start)
extern size_t printk_index;

int kprintf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
int fprintf(vnode_t *vnode, const char *fmt, ...);
int vfprintf(vnode_t *vnode, const char *fmt, va_list args);
int printf(const char *fmt, ...);
int fwrite(vnode_t *vnode, const void *buffer, size_t size);

#endif // LIB_PRINTF_H