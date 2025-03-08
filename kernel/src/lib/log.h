#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <lib/debug.h>
#include <lib/printf.h>

static int _warnings __attribute__((unused)) = 0;

#define _LOG(color, level, format, ...)                                              \
    do                                                                               \
    {                                                                                \
        printf("\033[1;%dm[%-5s]\033[0m " format "\n", color, level, ##__VA_ARGS__); \
    } while (0)

#define _LOG_W(color, level, format, ...)                                    \
    do                                                                       \
    {                                                                        \
        _warnings++;                                                         \
        printf("\033[1;%dm[%-5s]\033[0m [%s:%d] (Warning #%d) " format "\n", \
               color, level, __FILE__, __LINE__, _warnings, ##__VA_ARGS__);  \
    } while (0)

#define _LOG_P(color, level, format, ...)                                                                        \
    do                                                                                                           \
    {                                                                                                            \
        printf("\033[1;%dm[%-5s]\033[0m [%s:%d] " format "\n", color, level, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#if _DEBUG
#define debug(format, ...) _LOG_P(94, "DEBUG", format, ##__VA_ARGS__)
#else
#define debug(format, ...) (void)0
#endif

#if _TRACE
#define trace(format, ...) _LOG_P(95, "TRACE", format, ##__VA_ARGS__)
#else
#define trace(format, ...) (void)0
#endif

#if _HEAP_TRACE
#define la_trace(format, ...) _LOG_P(96, "TRACE", format, ##__VA_ARGS__)
#else
#define la_trace(format, ...) (void)0
#endif

#define info(format, ...) _LOG(92, "INFO", format, ##__VA_ARGS__)
#define warning(format, ...) _LOG_W(93, "WARN", format, ##__VA_ARGS__)
#define error(format, ...) _LOG_P(91, "ERROR", format, ##__VA_ARGS__)

#endif // LIB_LOG_H
