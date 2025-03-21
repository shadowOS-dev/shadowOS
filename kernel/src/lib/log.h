#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <lib/debug.h>
#include <lib/printf.h>

#define UPPER_STR(str)                          \
    do                                          \
    {                                           \
        for (int i = 0; str[i]; i++)            \
        {                                       \
            if (str[i] >= 'a' && str[i] <= 'z') \
            {                                   \
                str[i] = str[i] - 'a' + 'A';    \
            }                                   \
        }                                       \
    } while (0)

#define LOG_FMT_WRAP(level)        \
    ({                             \
        char level_copy[] = level; \
        UPPER_STR(level_copy);     \
        level_copy;                \
    })

static int _warnings __attribute__((unused)) = 0;

#define LOG_DEFAULT_FORMAT "[%-5s] "

#define LOG_CALLBACK(color, level, format, ...)                                \
    do                                                                         \
    {                                                                          \
        char level_copy[] = level;                                             \
        UPPER_STR(level_copy);                                                 \
        kprintf("\033[1;%dm" LOG_DEFAULT_FORMAT "\033[0m", color, level_copy); \
        kprintf(format, ##__VA_ARGS__);                                        \
        kprintf("\n");                                                         \
    } while (0)

#define LOG_CALLBACK_EW(color, level, format, ...)                             \
    do                                                                         \
    {                                                                          \
        char level_copy[] = level;                                             \
        UPPER_STR(level_copy);                                                 \
        kprintf("\033[1;%dm" LOG_DEFAULT_FORMAT "\033[0m", color, level_copy); \
        kprintf(format, ##__VA_ARGS__);                                        \
        kprintf("\n");                                                         \
        printf("\033[1;%dm" LOG_DEFAULT_FORMAT "\033[0m", color, level_copy);  \
        printf(format, ##__VA_ARGS__);                                         \
        printf("\n");                                                          \
    } while (0)

#define _LOG(color, level, format, ...) \
    LOG_CALLBACK(color, level, format, ##__VA_ARGS__)

#define _LOG_W(color, level, format, ...)                                                 \
    do                                                                                    \
    {                                                                                     \
        _warnings++;                                                                      \
        LOG_CALLBACK_EW(color, level, "(Warning #%d) " format, _warnings, ##__VA_ARGS__); \
    } while (0)

#define _LOG_P(color, level, format, ...) \
    LOG_CALLBACK(color, level, format, ##__VA_ARGS__)

#define _LOG_E(color, level, format, ...) \
    LOG_CALLBACK_EW(color, level, format, ##__VA_ARGS__)

#if _DEBUG
#define debug(format, ...) _LOG_P(94, "debug", format, ##__VA_ARGS__)
#else
#define debug(format, ...) (void)0
#endif

#if _TRACE
#define trace(format, ...) _LOG_P(95, "trace", format, ##__VA_ARGS__)
#else
#define trace(format, ...) (void)0
#endif

#if _HEAP_TRACE
#define la_trace(format, ...) _LOG_P(96, "trace", format, ##__VA_ARGS__)
#else
#define la_trace(format, ...) (void)0
#endif

#if _SYSCALL_TRACE
#define s_trace(format, ...) _LOG_P(97, "trace", format, ##__VA_ARGS__)
#else
#define s_trace(format, ...) (void)0
#endif

#define info(format, ...) _LOG(92, "info", format, ##__VA_ARGS__)
#define warning(format, ...) _LOG_W(93, "warn", format, ##__VA_ARGS__)
#define error(format, ...) _LOG_E(91, "error", format, ##__VA_ARGS__)

#endif // LIB_LOG_H
