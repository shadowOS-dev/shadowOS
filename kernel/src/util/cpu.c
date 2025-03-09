#include <util/cpu.h>

[[noreturn]] void hcf(void)
{
    __asm__ volatile("sti");
    for (;;)
        __asm__ volatile("hlt");
}

[[noreturn]] void hlt(void)
{
    for (;;)
        __asm__ volatile("hlt");
}

void cpuid(uint32_t eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile("cpuid" : "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(eax));
}