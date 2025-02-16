#include <mm/pmm.h>
#include <lib/log.h>
#include <stddef.h>
#include <util/cpu.h>
#include <lib/memory.h>

pmm_stack_t stack;
struct limine_memmap_response *_memmap;

void pmm_init(struct limine_memmap_response *memmap)
{
    uint64_t free_pages = 0;
    _memmap = memmap;

    if (_memmap == NULL)
    {
        error("Memory map response is NULL, halting");
        hcf();
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {
            debug("Usable entry at 0x%.16llx, size: 0x%.16llx", memmap->entries[i]->base, memmap->entries[i]->length);
            free_pages += DIV_ROUND_UP(memmap->entries[i]->length, PAGE_SIZE);
        }
    }

    uint64_t array_size = ALIGN_UP(free_pages * 8, PAGE_SIZE);
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->length >= array_size && entry->type == LIMINE_MEMMAP_USABLE)
        {
            stack.pages = (uint64_t *)HIGHER_HALF(entry->base);
            entry->length -= array_size;
            entry->base += array_size;
            break;
        }
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE)
            {
                stack.pages[stack.idx++] = entry->base + j;
            }
        }
    }

    stack.max = stack.idx;
    debug("Max Index: 0x%.16llx", stack.max);
    debug("Current Index: 0x%.16llx", stack.idx);
}

void *pmm_request_page()
{
    if (stack.idx == 0)
    {
        error("Out of memory");
        return NULL;
    }

    uint64_t page_addr = stack.pages[--stack.idx];
    memset(HIGHER_HALF(page_addr), 0, PAGE_SIZE);
    return (void *)page_addr;
}

void pmm_release_page(void *page)
{
    if (page == NULL)
    {
        warning("Attempt to release a NULL page");
        return;
    }

    if (stack.idx >= stack.max)
    {
        warning("Stack overflow attempt while releasing page");
        return;
    }

    stack.pages[stack.idx++] = (uint64_t)page;
}