#ifndef LIB_ASSERT_H
#define LIB_ASSERT_H

#include <lib/log.h>
#include <util/cpu.h>

#define assert(expr)                                                                        \
    do                                                                                      \
    {                                                                                       \
        if (!(expr))                                                                        \
        {                                                                                   \
            error("Assertion failed: (%s), file: %s, line: %d", #expr, __FILE__, __LINE__); \
            hlt();                                                                          \
        }                                                                                   \
    } while (0)

#define msg_assert(expr, msg)                                                                                 \
    do                                                                                                        \
    {                                                                                                         \
        if (!(expr))                                                                                          \
        {                                                                                                     \
            error("Assertion failed: (%s), message: %s, file: %s, line: %d", #expr, msg, __FILE__, __LINE__); \
            hlt();                                                                                            \
        }                                                                                                     \
    } while (0)

#endif // LIB_ASSERT_H