#ifndef LIB_DEBUG_H
#define LIB_DEBUG_H

#define BLOCK_START(kind)                                               \
    do                                                                  \
    {                                                                   \
        debug("\033[1m------ Starting Block: %s ------\033[0m", #kind); \
    } while (0);

#define BLOCK_END(kind)                                               \
    do                                                                \
    {                                                                 \
        debug("\033[1m------ Ending Block: %s ------\033[0m", #kind); \
    } while (0);

void debug_lib_init();

#endif // LIB_DEBUG_H