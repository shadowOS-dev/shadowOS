#ifndef SYS_GDT_H
#define SYS_GDT_H

#include <stdint.h>

// GDT Access Flags
#define GDT_ACCESS_PRESENT 0x80    // Segment is present
#define GDT_ACCESS_RING0 0x00      // Privilege Level 0 (Kernel)
#define GDT_ACCESS_RING3 0x60      // Privilege Level 3 (User)
#define GDT_ACCESS_SYSTEM 0x00     // System segment (e.g., TSS)
#define GDT_ACCESS_CODE 0x18       // Code segment (Executable)
#define GDT_ACCESS_DATA 0x10       // Data segment (Readable/Writable)
#define GDT_ACCESS_DIRECTION 0x04  // Direction bit (0 = Grows up, 1 = Grows down)
#define GDT_ACCESS_EXECUTABLE 0x08 // Executable segment
#define GDT_ACCESS_RW 0x02         // Readable for code, Writable for data
#define GDT_ACCESS_ACCESSED 0x01   // CPU sets this when accessed

// Common Access Flags
#define GDT_KERNEL_CODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW)
#define GDT_KERNEL_DATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA | GDT_ACCESS_RW)
#define GDT_USER_CODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW)
#define GDT_USER_DATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA | GDT_ACCESS_RW)
#define GDT_TSS 0xE9

// Granularity Flags
#define GDT_GRANULARITY_4K 0x80
#define GDT_GRANULARITY_32B 0x40
#define GDT_GRANULARITY_LONG_MODE 0x20
#define GDT_GRANULARITY_FLAT (GDT_GRANULARITY_4K | GDT_GRANULARITY_LONG_MODE)

typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_system_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) gdt_system_entry_t;

typedef struct gdt_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct tss_entry
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed)) tss_entry_t;

extern gdt_ptr_t gdt_ptr;

void gdt_init();
void gdt_flush(gdt_ptr_t gdt_ptr);
void tss_init(uint64_t rsp0);
extern void jump_user(uint64_t addr, uint64_t stack);

#endif // SYS_GDT_H
