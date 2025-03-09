#ifndef LIB_PRINTF_H
#define LIB_PRINTF_H

#include <stddef.h>
#include <stdarg.h>

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);

#endif // LIB_PRINTF_H