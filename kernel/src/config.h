#ifndef CONFIG_H
#define CONFIG_H

// Logging config
#define _DEBUG 1
#define _TRACE 1
#define _HEAP_TRACE 0

// Memory allocation config
#define PAGE_SIZE 0x1000
#define VMA_START PAGE_SIZE

// Test config
#define RAMFS_TEST_PATH "/root/welcome.txt"

#endif // CONFIG_H