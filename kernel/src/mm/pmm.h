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

extern uint64_t hhdm_offset;

#define PAGE_SIZE 0x1000

#define DIV_ROUND_UP(x, y) (((uint64_t)(x) + ((uint64_t)(y) - 1)) / (uint64_t)(y))
#define ALIGN_UP(x, y) (DIV_ROUND_UP(x, y) * (uint64_t)(y))
#define ALIGN_DOWN(x, y) (((uint64_t)(x) / (uint64_t)(y)) * (uint64_t)(y))

#define HIGHER_HALF(ptr) ((void *)((uint64_t)ptr) + hhdm_offset)
#define PHYSICAL(ptr) ((void *)((uint64_t)ptr) - hhdm_offset)

void pmm_init(struct limine_memmap_response *memmap);
void *pmm_request_page();
void pmm_release_page(void *page);

#endif // MM_PMM_H