#ifndef SYS_INTR_H
#define SYS_INTR_H

// thanks bonzo for fuling my skid addiction

#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed)) register_ctx
{
    uint64_t es, ds;
    uint64_t cr4, cr3, cr2, cr0;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rax, rbx, rcx, rdx, rsi, rdi;
    uint64_t rbp;
    uint64_t vector, err;
    uint64_t rip, cs, rflags, rsp, ss;
};

struct __attribute__((packed)) idt_entry
{
    uint16_t off_low;
    uint16_t sel;
    uint8_t ist;
    uint8_t attr;
    uint16_t off_mid;
    uint32_t off_high;
    uint32_t zero;
};

typedef void (*idt_intr_handler)(struct register_ctx *ctx);
#define IDT_INTERRUPT_GATE (0x8E)
#define IDT_TRAP_GATE (0x8F)
#define IDT_IRQ_BASE (0x20)

void idt_init();
void load_idt();
int idt_register_handler(size_t vector, idt_intr_handler handler);
void idt_default_interrupt_handler(struct register_ctx *ctx);
void kpanic(struct register_ctx *ctx, const char *fmt, ...);

#endif // SYS_INTR_H