#include <mm/vma.h>
#include <lib/memory.h>
#include <lib/log.h>

vma_context_t *vma_create_context(uint64_t *pagemap)
{
    trace("Creating VMA context with pagemap: 0x%.16llx", (uint64_t)pagemap);

    vma_context_t *ctx = (vma_context_t *)HIGHER_HALF(pmm_request_page());
    if (ctx == NULL)
    {
        error("Failed to allocate VMA context");
        return NULL;
    }
    trace("Allocated VMA context at 0x%.16llx", (uint64_t)ctx);
    memset(ctx, 0, sizeof(vma_context_t));
    trace("Zeroed out VMA context at 0x%.16llx", (uint64_t)ctx);

    ctx->root = (vma_region_t *)HIGHER_HALF(pmm_request_page());
    if (ctx->root == NULL)
    {
        error("Failed to allocate root region");
        pmm_release_page((void *)PHYSICAL(ctx));
        return NULL;
    }
    trace("Allocated root region at 0x%.16llx", (uint64_t)ctx->root);
    memset(ctx->root, 0, sizeof(vma_region_t));
    trace("Zeroed out root region at 0x%.16llx", (uint64_t)ctx->root);

    ctx->pagemap = pagemap;
    ctx->root->start = VMA_START;
    ctx->root->size = 0;

    trace("VMA context created at 0x%.16llx with root region at 0x%.16llx", (uint64_t)ctx, (uint64_t)ctx->root);
    return ctx;
}

void vma_destroy_context(vma_context_t *ctx)
{
    trace("Destroying VMA context at 0x%.16llx", (uint64_t)ctx);

    if (ctx->root == NULL || ctx->pagemap == NULL)
    {
        error("Invalid context or root passed to vma_destroy_context");
        return;
    }

    vma_region_t *region = ctx->root;
    while (region != NULL)
    {
        trace("Freeing region at 0x%.16llx", (uint64_t)region);
        vma_region_t *next = region->next;
        pmm_release_page((void *)PHYSICAL(region));
        region = next;
    }

    pmm_release_page((void *)PHYSICAL(ctx));
    debug("Destroyed VMA context at 0x%.16llx", (uint64_t)ctx);
}

void *vma_alloc(vma_context_t *ctx, uint64_t size, uint64_t flags)
{
    trace("Allocating VMA region in context 0x%.16llx, size: 0x%.16llx, flags: 0x%.16llx", (uint64_t)ctx, size, flags);

    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
    {
        error("Invalid context or root passed to vma_alloc");
        return NULL;
    }

    vma_region_t *region = ctx->root->next;
    vma_region_t *new_region;
    vma_region_t *last_region;

    if (region == NULL)
    {
        new_region = (vma_region_t *)HIGHER_HALF(pmm_request_page());
        if (new_region == NULL)
        {
            error("Failed to allocate new VMA region");
            return NULL;
        }

        memset(new_region, 0, sizeof(vma_region_t));
        last_region = ctx->root;
        trace("Created new region at 0x%.16llx", (uint64_t)new_region);
        goto skip;
    }

    while (region != ctx->root)
    {
        trace("Checking region at 0x%.16llx with start: 0x%.16llx and size: 0x%.16llx",
              (uint64_t)region, region->start, region->size);

        if (region->start + (region->size * PAGE_SIZE) - region->next->start >= size)
        {
            new_region = (vma_region_t *)HIGHER_HALF(pmm_request_page());
            if (new_region == NULL)
            {
                error("Failed to allocate new VMA region");
                return NULL;
            }

            memset(new_region, 0, sizeof(vma_region_t));
            new_region->size = size;
            new_region->flags = flags;
            new_region->start = region->start + (region->size * PAGE_SIZE);
            new_region->next = region->next;
            new_region->prev = region;
            region->next = new_region;

            trace("Allocated new region at 0x%.16llx with start: 0x%.16llx, size: 0x%.16llx",
                  (uint64_t)new_region, new_region->start, new_region->size);

            for (uint64_t i = 0; i < ALIGN_UP(new_region->size, PAGE_SIZE) / PAGE_SIZE; i++)
            {
                uint64_t page = (uint64_t)pmm_request_page();
                if (page == 0)
                {
                    error("Failed to allocate physical memory for VMA region");
                    return NULL;
                }
                vmm_map(ctx->pagemap, new_region->start + (i * PAGE_SIZE), page, new_region->flags);
                trace("Mapped page 0x%.16llx at virtual address 0x%.16llx", page, new_region->start + (i * PAGE_SIZE));
            }

            return (void *)new_region->start;
        }
        region = region->next;
    }

    new_region = (vma_region_t *)HIGHER_HALF(pmm_request_page());
    if (new_region == NULL)
    {
        error("Failed to allocate new VMA region");
        return NULL;
    }

    memset(new_region, 0, sizeof(vma_region_t));

    last_region = ctx->root;
    while (last_region->next != NULL)
    {
        last_region = last_region->next;
    }

skip:
    new_region->start = last_region->start + (last_region->size * PAGE_SIZE);
    new_region->size = size;
    new_region->flags = flags;
    new_region->next = NULL;
    new_region->prev = last_region;
    last_region->next = new_region;

    trace("New region allocated at 0x%.16llx with start: 0x%.16llx, size: 0x%.16llx",
          (uint64_t)new_region, new_region->start, new_region->size);

    for (uint64_t i = 0; i < ALIGN_UP(new_region->size, PAGE_SIZE) / PAGE_SIZE; i++)
    {
        uint64_t page = (uint64_t)pmm_request_page();
        if (page == 0)
        {
            error("Failed to allocate physical memory for VMA region");
            return NULL;
        }
        vmm_map(ctx->pagemap, new_region->start + (i * PAGE_SIZE), page, new_region->flags);
        trace("Mapped page 0x%.16llx at virtual address 0x%.16llx", page, new_region->start + (i * PAGE_SIZE));
    }

    return (void *)new_region->start;
}

void vma_free(vma_context_t *ctx, void *ptr)
{
    trace("Freeing VMA region in context 0x%.16llx at address 0x%.16llx", (uint64_t)ctx, (uint64_t)ptr);

    if (ctx == NULL)
    {
        error("Invalid context passed to vma_free");
        return;
    }

    vma_region_t *region = ctx->root;
    while (region != NULL)
    {
        if (region->start == (uint64_t)ptr)
        {
            trace("Found region to free at 0x%.16llx", (uint64_t)region);
            break;
        }
        region = region->next;
    }

    if (region == NULL)
    {
        error("Unable to find region to free at address 0x%.16llx", (uint64_t)ptr);
        return;
    }

    vma_region_t *prev = region->prev;
    vma_region_t *next = region->next;

    for (uint64_t i = 0; i < region->size; i++)
    {
        uint64_t virt = region->start + (i * PAGE_SIZE);
        uint64_t phys = virt_to_phys(kernel_pagemap, virt);

        if (phys != 0)
        {
            trace("Freeing page %d, virt: 0x%.16llx, phys: 0x%.16llx", i, virt, phys);
            pmm_release_page((void *)phys);
            vmm_unmap(ctx->pagemap, virt);
        }
    }

    if (prev != NULL)
    {
        prev->next = next;
    }

    if (next != NULL)
    {
        next->prev = prev;
    }

    if (region == ctx->root)
    {
        ctx->root = next;
    }

    if (region != NULL)
    {
        pmm_release_page((void *)PHYSICAL(region));
        trace("Released region at 0x%.16llx", (uint64_t)region);
    }
}
