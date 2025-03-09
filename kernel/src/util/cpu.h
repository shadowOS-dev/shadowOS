#ifndef UTIL_CPU_H
#define UTIL_CPU_H

#include <stdint.h>

[[noreturn]] void hcf(void);
[[noreturn]] void hlt(void);
void cpuid(uint32_t eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

#endif // UTIL_CPU_H
