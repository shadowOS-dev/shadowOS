#include <mm/pmm.h>
#include <lib/log.h>
#include <stddef.h>
#include <util/cpu.h>
#include <lib/memory.h>
#include <stdbool.h>

pmm_stack_t stack;
struct limine_memmap_response *_memmap;

void pmm_init(struct limine_memmap_response *memmap)
{
    uint64_t free_pages = 0;
    uint64_t array_size;
    _memmap = memmap;

    if (_memmap == NULL)
    {
        error("Memory map response is NULL, halting");
        hcf();
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            free_pages += DIV_ROUND_UP(entry->length, PAGE_SIZE);
        }
    }

    array_size = ALIGN_UP(free_pages * 8, PAGE_SIZE);

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
}

void *pmm_request_page()
{
    if (stack.idx == 0)
    {
        error("Out of memory");
        return NULL;
    }

    if (stack.idx - 1 == (uint64_t)-1) // not a good way to check for underflow
    {
        warning("Stack underflow detected");
        return NULL;
    }

    uint64_t page_addr = stack.pages[--stack.idx];

    if ((page_addr & (PAGE_SIZE - 1)) != 0)
    {
        warning("Requested page address is not aligned to page size: 0x%.16llx", page_addr);
    }

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

    bool valid_page = false;

    for (uint64_t i = 0; i < _memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = _memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE &&
            (uint64_t)page >= entry->base && (uint64_t)page < entry->base + entry->length)
        {
            valid_page = true;
            break;
        }
    }

    if (!valid_page)
    {
        warning("Attempt to release an invalid or out-of-bounds page at 0x%.16llx", (uint64_t)page);
        return;
    }

    if (stack.idx >= stack.max)
    {
        warning("Stack overflow attempt while releasing page");
        return;
    }

    stack.pages[stack.idx++] = (uint64_t)page;
}
