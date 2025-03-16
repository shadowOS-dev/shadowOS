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
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
    {
        error("Invalid context or root passed to vma_alloc");
        return NULL;
    }

    vma_region_t *region = ctx->root;
    vma_region_t *new_region;
    vma_region_t *last_region = ctx->root;

    while (region != NULL)
    {
        if (region->next == NULL || region->start + region->size < region->next->start)
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
            new_region->start = region->start + region->size;
            new_region->next = region->next;
            new_region->prev = region;
            region->next = new_region;

            for (uint64_t i = 0; i < size; i++)
            {
                uint64_t page = (uint64_t)pmm_request_page();
                if (page == 0)
                {
                    error("Failed to allocate physical memory for VMA region");
                    return NULL;
                }

                vmm_map(ctx->pagemap, new_region->start + i * PAGE_SIZE, page, new_region->flags);
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

    last_region->next = new_region;
    new_region->prev = last_region;
    new_region->start = last_region->start + last_region->size;
    new_region->size = size;
    new_region->flags = flags;
    new_region->next = NULL;

    for (uint64_t i = 0; i < size; i++)
    {
        uint64_t page = (uint64_t)pmm_request_page();
        if (page == 0)
        {
            error("Failed to allocate physical memory for VMA region");
            return NULL;
        }

        vmm_map(ctx->pagemap, new_region->start + i * PAGE_SIZE, page, new_region->flags);
    }

    return (void *)new_region->start;
}

void vma_free(vma_context_t *ctx, void *ptr)
{
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
        uint64_t virt = region->start + i * PAGE_SIZE;
        uint64_t phys = virt_to_phys(kernel_pagemap, virt);

        if (phys != 0)
        {
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

    pmm_release_page((void *)PHYSICAL(region));
}

void vma_dump_context(vma_context_t *ctx)
{
    if (ctx == NULL || ctx->root == NULL)
    {
        error("Invalid VMA context or root region");
        return;
    }

    trace("Dumping VMA context at 0x%.16llx", (uint64_t)ctx);
    trace("Context details:");
    trace("  - Root region at 0x%.16llx", (uint64_t)ctx->root);
    trace("  - Pagemap address at 0x%.16llx", (uint64_t)ctx->pagemap);

    vma_region_t *region = ctx->root;
    int region_count = 0;

    while (region != NULL)
    {
        trace("Region %d: start=0x%.16llx, size=%llu pages, flags=0x%.8llx",
              region_count, region->start, region->size, region->flags);

        uint64_t region_end = region->start + (region->size * PAGE_SIZE);
        trace("  - Region start address: 0x%.16llx", region->start);
        trace("  - Region end address: 0x%.16llx", region_end);
        trace("  - Region size in bytes: %llu", region->size * PAGE_SIZE);

        trace("  - Pages mapped for region: ");
        for (uint64_t i = 0; i < region->size; i++)
        {
            uint64_t virt_address = region->start + (i * PAGE_SIZE);
            uint64_t phys_address = virt_to_phys(ctx->pagemap, virt_address);
            trace("    - Virtual: 0x%.16llx -> Physical: 0x%.16llx", virt_address, phys_address);
        }

        if (region->prev != NULL)
        {
            trace("  - Previous region: start=0x%.16llx, addr=0x%.16llx",
                  region->prev->start, (uint64_t)region->prev);
        }
        else
        {
            trace("  - Previous region: NULL");
        }

        if (region->next != NULL)
        {
            trace("  - Next region: start=0x%.16llx, addr=0x%.16llx",
                  region->next->start, (uint64_t)region->next);
        }
        else
        {
            trace("  - Next region: NULL");
        }

        trace("  - Region flags: ");
        if (region->flags & VMM_PRESENT)
            trace("    - Readable");
        if (region->flags & VMM_WRITE)
            trace("    - Writable");
        if (region->flags & VMM_USER)
            trace("    - User");
        if (region->flags & VMM_NX)
            trace("    - NX");

        region_count++;
        region = region->next;
    }

    trace("End of VMA context dump");
}