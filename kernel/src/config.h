#ifndef CONFIG_H
#define CONFIG_H

// Logging config
#define _DEBUG 0
#define _TRACE 0

// Memory allocation config
#define PAGE_SIZE 0x1000
#define VMA_START PAGE_SIZE

// Testing configuration
#define KMALLOC_TEST 1
#define KMA_TEST_COUNT 10

#endif // CONFIG_H