#include <sys/gdt.h>
#include <lib/log.h>
#include <lib/memory.h>
#include <util/cpu.h>

gdt_entry_t gdt[7];
gdt_ptr_t gdt_ptr;
tss_entry_t tss;

void gdt_init()
{
    trace("Initializing GDT...");

    gdt[0] = (gdt_entry_t){0, 0, 0, 0x00, 0x00, 0}; // Null descriptor     0x0
    gdt[1] = (gdt_entry_t){0, 0, 0, 0x9A, 0xA0, 0}; // Kernel code segment 0x8
    gdt[2] = (gdt_entry_t){0, 0, 0, 0x92, 0xA0, 0}; // Kernel data segment 0x10
    gdt[3] = (gdt_entry_t){0, 0, 0, 0xFA, 0x20, 0}; // User code segment   0x18
    gdt[4] = (gdt_entry_t){0, 0, 0, 0xF2, 0x00, 0}; // User data segment   0x20

    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base = (uint64_t)&gdt;

    trace("GDT limit: 0x%.4x, base: 0x%.16llx", gdt_ptr.limit, gdt_ptr.base);

    gdt_flush(gdt_ptr);
    trace("GDT initialized successfully.");
}

void flush_tss(void);
void tss_init(uint64_t stack)
{
    trace("Initializing TSS with RSP0 = 0x%.16llx", stack);

    memset(&tss, 0, sizeof(tss_entry_t));

    tss.rsp0 = stack;
    tss.io_map_base = sizeof(tss_entry_t);

    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;

    trace("TSS base address: 0x%.16llx, limit: 0x%.8x", base, limit);

    gdt_system_entry_t tss_entry = {
        .limit_low = limit & 0xFFFF,
        .base_low = base & 0xFFFF,
        .base_middle = (base >> 16) & 0xFF,
        .access = 0xE9,
        .granularity = (limit >> 16) & 0x0F,
        .base_high = (base >> 24) & 0xFF,
        .base_upper = base >> 32,
        .reserved = 0,
    };
    memcpy(&gdt[5], &tss_entry, sizeof(gdt_system_entry_t));

    gdt_flush(gdt_ptr);
    flush_tss();
}

void gdt_flush(gdt_ptr_t gdt_ptr)
{
    trace("Flushing GDT to CPU...");

    __asm__ volatile(
        "mov %0, %%rdi\n"
        "lgdt (%%rdi)\n"
        "push $0x8\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%fs\n"
        :
        : "r"(&gdt_ptr)
        : "memory");

    trace("GDT flushed successfully.");
}
