#ifndef CONFIG_H
#define CONFIG_H

// Logging config
#define _DEBUG 0
#define _TRACE 0
#define _HEAP_TRACE 0

// Memory allocation config
#define PAGE_SIZE 0x1000
#define VMA_START PAGE_SIZE

// Misc
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif // CONFIG_H