#include <util/cpu.h>

[[noreturn]] void hcf(void) {
    __asm__ volatile("sti");
    for(;;) __asm__ volatile("hlt");
}

[[noreturn]] void hlt(void) {
    for(;;) __asm__ volatile("hlt");
}
