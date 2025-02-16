#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <lib/printf.h>

#define _LOG(color, level, format, ...)                                              \
    do                                                                               \
    {                                                                                \
        printf("\033[1;%dm[%-8s]\033[0m " format "\n", color, level, ##__VA_ARGS__); \
    } while (0)

#define _LOG_ERR(color, level, format, ...)                                                                     \
    do                                                                                                          \
    {                                                                                                           \
        printf("\033[1;%dm[%-8s]\033[0m %s:%d  " format "\n", color, level, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#ifdef _DEBUG
#define debug(format, ...) _LOG_ERR(94, "DEBUG", format, ##__VA_ARGS__)
#else
#define debug(format, ...) (void)0
#endif

#define info(format, ...) _LOG(92, "INFO", format, ##__VA_ARGS__)
#define warning(format, ...) _LOG(93, "WARNING", format, ##__VA_ARGS__)
#define error(format, ...) _LOG_ERR(91, "ERROR", format, ##__VA_ARGS__)

#endif // LIB_LOG_H
