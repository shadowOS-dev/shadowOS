#ifndef MM_VMA_H
#define MM_VMA_H

#include <mm/vmm.h>
#include <mm/pmm.h>
#include <stdint.h>

typedef struct vma_region
{
    uint64_t start;
    uint64_t size;
    uint64_t flags;
    struct vma_region *next;
    struct vma_region *prev;
} vma_region_t;

typedef struct vma_context
{
    uint64_t *pagemap;
    vma_region_t *root;
} vma_context_t;

vma_context_t *vma_create_context(uint64_t *pagemap);
void vma_destroy_context(vma_context_t *ctx);
void *vma_alloc(vma_context_t *ctx, uint64_t size, uint64_t flags);
void vma_free(vma_context_t *ctx, void *ptr);

#endif // MM_VMA_H