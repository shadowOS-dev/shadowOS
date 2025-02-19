#ifndef LIB_LOG_H
#define LIB_LOG_H

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

#define info(format, ...) _LOG(92, "INFO", format, ##__VA_ARGS__)
#define warning(format, ...) _LOG_W(93, "WARN", format, ##__VA_ARGS__)
#define error(format, ...) _LOG_P(91, "ERROR", format, ##__VA_ARGS__)

// TODO: Move to somewhere else kinda like <lib/debug.h>
#define BLOCK_START(kind)                                               \
    do                                                                  \
    {                                                                   \
        trace("\033[1m------ Starting Block: %s ------\033[0m", #kind); \
    } while (0);

#define BLOCK_END(kind)                                               \
    do                                                                \
    {                                                                 \
        trace("\033[1m------ Ending Block: %s ------\033[0m", #kind); \
    } while (0);

#endif // LIB_LOG_H
