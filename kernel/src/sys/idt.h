#ifndef SYS_IDT_H
#define SYS_IDT_H

#include <stdint.h>

#define IDT_ENTRY_COUNT 0xFF

typedef struct idt_entry
{
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

typedef struct int_frame
{
    uint64_t ds;
    uint64_t cr2;
    uint64_t cr3;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t vector;
    uint64_t err;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) int_frame_t;

extern idt_ptr_t idt_ptr;
extern idt_entry_t idt_entries[IDT_ENTRY_COUNT];

typedef void (*interrupt_handler_t)(int_frame_t *frame);
void idt_init();
void idt_load(uint64_t);
void idt_set_gate(idt_entry_t idt[], int num, uint64_t base, uint16_t segment, uint8_t flags);

#endif // SYS_IDT_H