#ifndef MM_PMM_H
#define MM_PMM_H

#include <stdint.h>
#include <limine.h>

typedef struct pmm_stack
{
    uint64_t *pages;
    uint64_t idx;
    uint64_t max;
} pmm_stack_t;

#define BYTES_TO_KB(bytes) ((bytes) / 1024 + (((bytes) % 1024) >= 512 ? 1 : 0))
#define BYTES_TO_MB(bytes) (BYTES_TO_KB(bytes) / 1024 + ((BYTES_TO_KB(bytes) % 1024) >= 512 ? 1 : 0))
#define BYTES_TO_GB(bytes) (BYTES_TO_MB(bytes) / 1024 + ((BYTES_TO_MB(bytes) % 1024) >= 512 ? 1 : 0))
#define BYTES_TO_TB(bytes) (BYTES_TO_GB(bytes) / 1024 + ((BYTES_TO_GB(bytes) % 1024) >= 512 ? 1 : 0))

extern uint64_t hhdm_offset;

#define DIV_ROUND_UP(x, y) (((uint64_t)(x) + ((uint64_t)(y) - 1)) / (uint64_t)(y))
#define ALIGN_UP(x, y) (DIV_ROUND_UP(x, y) * (uint64_t)(y))
#define ALIGN_DOWN(x, y) (((uint64_t)(x) / (uint64_t)(y)) * (uint64_t)(y))

#define HIGHER_HALF(ptr) ((void *)((uint64_t)ptr) + hhdm_offset)
#define PHYSICAL(ptr) ((void *)((uint64_t)ptr) - hhdm_offset)

void pmm_init(struct limine_memmap_response *memmap);
void pmm_vmm_cleanup(struct limine_memmap_response *memmap);
void *pmm_request_page();
void pmm_release_page(void *page);
uint64_t pmm_get_free_memory();

#endif // MM_PMM_H