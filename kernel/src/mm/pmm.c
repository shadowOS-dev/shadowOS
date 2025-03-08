#include <mm/pmm.h>
#include <lib/log.h>
#include <stddef.h>
#include <util/cpu.h>
#include <lib/memory.h>
#include <stdbool.h>
#include <mm/kmalloc.h>

#define CACHE_SIZE 128
#define SECONDARY_CACHE_SIZE 64

typedef struct page_cache_entry
{
    uint64_t page_addr;
    bool is_valid;
    uint64_t access_count;
} page_cache_entry_t;

pmm_stack_t stack;
struct limine_memmap_response *_memmap;

static page_cache_entry_t valid_page_cache[CACHE_SIZE] = {0};
static uint64_t cache_head = 0;
static page_cache_entry_t secondary_cache[SECONDARY_CACHE_SIZE] = {0};

bool reclaimed = false;

static inline uint64_t hash_page(uint64_t addr)
{
    const uint64_t prime1 = 0x9e3779b97f4a7c15;
    const uint64_t prime2 = 0x7f4a7c159e3779b9;
    uint64_t hash = addr;

    hash = (hash ^ (hash >> 30)) * prime1;
    hash = (hash ^ (hash >> 27)) * prime2;
    hash = hash ^ (hash >> 31);

    return hash % CACHE_SIZE;
}

static bool is_in_secondary_cache(uint64_t page_addr)
{
    uint64_t hash = hash_page(page_addr);
    if (secondary_cache[hash].is_valid && secondary_cache[hash].page_addr == page_addr)
    {
        trace("Page address 0x%.16llx found in secondary cache", page_addr);
        return true;
    }
    return false;
}

static void update_cache_access(uint64_t page_addr)
{
    uint64_t cache_idx = hash_page(page_addr);

    if (valid_page_cache[cache_idx].is_valid && valid_page_cache[cache_idx].page_addr == page_addr)
    {
        valid_page_cache[cache_idx].access_count++;
        trace("Updated access count for page 0x%.16llx in primary cache", page_addr);
        return;
    }

    if (is_in_secondary_cache(page_addr))
    {
        for (uint64_t i = 0; i < SECONDARY_CACHE_SIZE; ++i)
        {
            if (secondary_cache[i].page_addr == page_addr)
            {
                uint64_t idx = hash_page(page_addr);
                valid_page_cache[idx] = secondary_cache[i];
                secondary_cache[i].is_valid = false;
                valid_page_cache[idx].access_count = 1;
                trace("Page 0x%.16llx moved from secondary to primary cache", page_addr);
                break;
            }
        }
    }
}

static bool is_page_valid(uint64_t page_addr)
{
    uint64_t cache_idx = hash_page(page_addr);

    if (valid_page_cache[cache_idx].is_valid && valid_page_cache[cache_idx].page_addr == page_addr)
    {
        update_cache_access(page_addr);
        trace("Page 0x%.16llx is valid in primary cache, access count: %d", page_addr, valid_page_cache[cache_idx].access_count);
        return true;
    }

    for (uint64_t i = 0; i < _memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = _memmap->entries[i];
        if ((entry->type == LIMINE_MEMMAP_USABLE || (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE && reclaimed)) &&
            page_addr >= entry->base && page_addr < entry->base + entry->length)
        {
            update_cache_access(page_addr);
            return true;
        }
    }

    trace("Page 0x%.16llx is not valid", page_addr);
    return false;
}

#if _TRACE
void trace_size(const char *label, uint64_t size_in_bytes)
{
    uint64_t size_in_kb = BYTES_TO_KB(size_in_bytes);
    uint64_t size_in_mb = BYTES_TO_MB(size_in_bytes);
    uint64_t size_in_gb = BYTES_TO_GB(size_in_bytes);
    uint64_t size_in_tb = BYTES_TO_TB(size_in_bytes);

    trace("%s:", label);
    trace(" - %llu KB", size_in_kb);
    trace(" - %llu MB", size_in_mb);
    trace(" - %llu GB", size_in_gb);
    trace(" - %llu TB", size_in_tb);
}
#else
#define trace_size(label, size_in_bytes)
#endif

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

    uint64_t cached_count = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->length >= array_size && entry->type == LIMINE_MEMMAP_USABLE)
        {
            stack.pages = (uint64_t *)HIGHER_HALF(entry->base);
            entry->length -= array_size;
            entry->base += array_size;

            for (uint64_t j = 0; j < CACHE_SIZE && cached_count < free_pages; ++j)
            {
                valid_page_cache[j].page_addr = entry->base + (j * PAGE_SIZE);
                valid_page_cache[j].is_valid = true;
                valid_page_cache[j].access_count = 0;
                cached_count++;
                trace("Cached page 0x%.16llx, current cache count: %d", valid_page_cache[j].page_addr, cached_count);
            }

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

    uint64_t reusable_pages = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
        {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE)
            {
                reusable_pages++;
            }
        }
    }

    stack.max = stack.idx;

    trace("PMM initialization complete. Total free pages: %llu, total cached pages: %llu", free_pages, cached_count);

    trace_size("Free memory", free_pages * PAGE_SIZE);
    trace_size("Cached memory", cached_count * PAGE_SIZE);
    trace_size("Reusable memory", reusable_pages * PAGE_SIZE);
    trace_size("Total memory", (free_pages + cached_count) * PAGE_SIZE);
}

// Not safe cleanup, but idc i have free will. FUCK I LOVE MEMORY OVERFLOWS
void pmm_vmm_cleanup(struct limine_memmap_response *memmap)
{
    (void)memmap;
    uint64_t reclaimed_count = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
        {
            trace("Reclaiming bootloader memory at 0x%.16llx, length: %llu", entry->base, entry->length);

            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE)
            {
                stack.pages[stack.idx++] = entry->base + j;
                reclaimed_count++;
                pmm_release_page((void *)(entry->base + j));
                trace("Reclaimed and released page at 0x%.16llx, current stack size: %llu", entry->base + j, stack.idx);
            }
        }
    }

    stack.max = stack.idx;

    trace("VMM cleanup complete. Reclaimed %llu pages", reclaimed_count);
}

void *pmm_request_page()
{
    if (stack.idx == 0)
    {
        error("Out of memory");
        return NULL;
    }

    if (stack.idx - 1 == (uint64_t)-1)
    {
        warning("Stack underflow detected");
        return NULL;
    }

    uint64_t page_addr = stack.pages[--stack.idx];

    if (!is_page_valid(page_addr))
    {
        warning("Requested page at 0x%.16llx is not valid or out-of-bounds", page_addr);
        return NULL;
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

    uint64_t page_addr = (uint64_t)page;

    if (!is_page_valid(page_addr))
    {
        warning("Attempt to release an invalid or out-of-bounds page at 0x%.16llx", page_addr);
        return;
    }

    if (stack.idx >= stack.max)
    {
        warning("Stack overflow attempt while releasing page");
        return;
    }

    uint64_t cache_idx = hash_page(page_addr);
    valid_page_cache[cache_idx].page_addr = page_addr;
    valid_page_cache[cache_idx].is_valid = true;
    valid_page_cache[cache_idx].access_count = 0;

    cache_head = (cache_head + 1) % CACHE_SIZE;
    trace("Released page 0x%.16llx and updated cache, new cache head: %d", page_addr, cache_head);

    stack.pages[stack.idx++] = page_addr;
}

uint64_t pmm_get_free_memory()
{
    return stack.idx * PAGE_SIZE;
}