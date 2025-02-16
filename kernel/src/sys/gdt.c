#include <sys/gdt.h>

gdt_entry_t gdt[5];
gdt_ptr_t gdt_ptr;

void gdt_init()
{
    gdt[0] = (gdt_entry_t){0, 0, 0, 0x00, 0x00, 0};
    gdt[1] = (gdt_entry_t){0, 0, 0, 0x9A, 0xA0, 0};
    gdt[2] = (gdt_entry_t){0, 0, 0, 0x92, 0xA0, 0};
    gdt[3] = (gdt_entry_t){0, 0, 0, 0xFA, 0xA0, 0};
    gdt[4] = (gdt_entry_t){0, 0, 0, 0xF2, 0xA0, 0};
    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base = (uint64_t)&gdt;
    gdt_flush(gdt_ptr);
}

void gdt_flush(gdt_ptr_t gdt_ptr)
{
    __asm__ volatile("mov %0, %%rdi\n"
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
}