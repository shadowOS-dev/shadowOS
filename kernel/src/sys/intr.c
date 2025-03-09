#include <sys/intr.h>
#include <lib/log.h>
#include <util/cpu.h>
#include <lib/memory.h>

struct idt_entry __attribute__((aligned(16))) idt_descriptor[256] = {0};
idt_intr_handler real_handlers[256] = {0};
extern uint64_t stubs[];

struct __attribute__((packed)) idt_ptr
{
    uint16_t limit;
    uint64_t base;
};

struct idt_ptr idt_ptr = {sizeof(idt_descriptor) - 1, (uint64_t)&idt_descriptor};

static const char *strings[32] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable-Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid opcode",
    "Device (FPU) not available",
    "Double Fault",
    "RESERVED VECTOR",
    "Invalid TSS",
    "Segment not present",
    "Stack Segment Fault",
    "General Protection Fault ",
    "Page Fault ",
    "RESERVED VECTOR",
    "x87 FP Exception",
    "Alignment Check",
    "Machine Check (Internal Error)",
    "SIMD FP Exception",
    "Virtualization Exception",
    "Control  Protection Exception",
    "RESERVED VECTOR",
    "RESERVED VECTOR",
    "RESERVED VECTOR",
    "RESERVED VECTOR",
    "RESERVED VECTOR",
    "RESERVED VECTOR",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "RESERVED VECTOR"};

static void capture_regs(struct register_ctx *context)
{
    __asm__ volatile(
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1\n\t"
        "movq %%rcx, %2\n\t"
        "movq %%rdx, %3\n\t"
        "movq %%rsi, %4\n\t"
        "movq %%rdi, %5\n\t"
        "movq %%rbp, %6\n\t"
        "movq %%r8,  %7\n\t"
        "movq %%r9,  %8\n\t"
        "movq %%r10, %9\n\t"
        "movq %%r11, %10\n\t"
        "movq %%r12, %11\n\t"
        "movq %%r13, %12\n\t"
        "movq %%r14, %13\n\t"
        "movq %%r15, %14\n\t"
        : "=m"(context->rax), "=m"(context->rbx), "=m"(context->rcx), "=m"(context->rdx),
          "=m"(context->rsi), "=m"(context->rdi), "=m"(context->rbp), "=m"(context->r9),
          "=m"(context->r9), "=m"(context->r10), "=m"(context->r11), "=m"(context->r12),
          "=m"(context->r13), "=m"(context->r14), "=m"(context->r15)
        :
        : "memory");

    __asm__ volatile(
        "movq %%cs,  %0\n\t"
        "movq %%ss,  %1\n\t"
        "movq %%es,  %2\n\t"
        "movq %%ds,  %3\n\t"
        "movq %%cr0, %4\n\t"
        "movq %%cr2, %5\n\t"
        "movq %%cr3, %6\n\t"
        "movq %%cr4, %7\n\t"
        : "=r"(context->cs), "=r"(context->ss), "=r"(context->es), "=r"(context->ds),
          "=r"(context->cr0), "=r"(context->cr2), "=r"(context->cr3), "=r"(context->cr4)
        :
        : "memory");

    __asm__ volatile(
        "movq %%rsp, %0\n\t"
        "pushfq\n\t"
        "popq %1\n\t"
        : "=r"(context->rsp), "=r"(context->rflags)
        :
        : "memory");

    context->rip = (uint64_t)__builtin_return_address(0);
}

void kpanic(struct register_ctx *ctx, const char *fmt, ...)
{
    struct register_ctx regs;
    if (ctx == NULL)
    {
        capture_regs(&regs);
        regs.err = 0xDEADBEEF;
        regs.vector = 0x0;
    }
    else
    {
        memcpy(&regs, ctx, sizeof(struct register_ctx));
    }

    printf("\n========== KERNEL PANIC ==========\n\n");
    printf("PANIC OCCURRED: ");
    if (fmt)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
    else
    {
        printf("%s", strings[regs.vector]);
    }

    printf("\n\n");

    printf("<<< REGISTER DUMP >>>\n");

    printf("| %-12s | 0x%016llx |\n", "rax", regs.rax);
    printf("| %-12s | 0x%016llx |\n", "rbx", regs.rbx);
    printf("| %-12s | 0x%016llx |\n", "rcx", regs.rcx);
    printf("| %-12s | 0x%016llx |\n", "rdx", regs.rdx);
    printf("| %-12s | 0x%016llx |\n", "rsi", regs.rsi);
    printf("| %-12s | 0x%016llx |\n", "rdi", regs.rdi);
    printf("| %-12s | 0x%016llx |\n", "rbp", regs.rbp);
    printf("| %-12s | 0x%016llx |\n", "r8", regs.r8);
    printf("| %-12s | 0x%016llx |\n", "r9", regs.r9);
    printf("| %-12s | 0x%016llx |\n", "r10", regs.r10);
    printf("| %-12s | 0x%016llx |\n", "r11", regs.r11);
    printf("| %-12s | 0x%016llx |\n", "r12", regs.r12);
    printf("| %-12s | 0x%016llx |\n", "r13", regs.r13);
    printf("| %-12s | 0x%016llx |\n", "r14", regs.r14);
    printf("| %-12s | 0x%016llx |\n", "r15", regs.r15);

    printf("\n[SEGMENT REGISTERS]\n");
    printf("| %-12s | 0x%016llx |\n", "es", regs.es);
    printf("| %-12s | 0x%016llx |\n", "ds", regs.ds);

    printf("\n[CONTROL REGISTERS]\n");
    printf("| %-12s | 0x%016llx |\n", "cr0", regs.cr0);
    printf("| %-12s | 0x%016llx |\n", "cr2", regs.cr2);
    printf("| %-12s | 0x%016llx |\n", "cr3", regs.cr3);
    printf("| %-12s | 0x%016llx |\n", "cr4", regs.cr4);

    printf("\n[FINAL SYSTEM STATE]\n");
    printf("| %-12s | 0x%016llx |\n", "vector", regs.vector);
    printf("| %-12s | 0x%016llx |\n", "err", regs.err);
    printf("| %-12s | 0x%016llx |\n", "rip", regs.rip);
    printf("| %-12s | 0x%016llx |\n", "cs", regs.cs);
    printf("| %-12s | 0x%016llx |\n", "rflags", regs.rflags);
    printf("| %-12s | 0x%016llx |\n", "rsp", regs.rsp);
    printf("| %-12s | 0x%016llx |\n", "ss", regs.ss);

    printf("\n!!! SYSTEM CRASHED !!!\n");

    hcf();
}

void idt_default_interrupt_handler(struct register_ctx *ctx)
{
    kpanic(ctx, NULL);
}

#define SET_GATE(interrupt, base, flags)                                    \
    do                                                                      \
    {                                                                       \
        idt_descriptor[(interrupt)].off_low = (base) & 0xFFFF;              \
        idt_descriptor[(interrupt)].sel = 0x8;                              \
        idt_descriptor[(interrupt)].ist = 0;                                \
        idt_descriptor[(interrupt)].attr = (flags);                         \
        idt_descriptor[(interrupt)].off_mid = ((base) >> 16) & 0xFFFF;      \
        idt_descriptor[(interrupt)].off_high = ((base) >> 32) & 0xFFFFFFFF; \
        idt_descriptor[(interrupt)].zero = 0;                               \
    } while (0)

void idt_init()
{
    for (int i = 0; i < 32; i++)
    {
        SET_GATE(i, stubs[i], IDT_TRAP_GATE);
        real_handlers[i] = idt_default_interrupt_handler;
    }

    for (int i = 32; i < 256; i++)
    {
        SET_GATE(i, stubs[i], IDT_INTERRUPT_GATE);
    }

    trace("Interrupt Descriptor Table (IDT)");
    trace("======================================================================================");
    trace("| Vec | Offset             | Selector  | Type | IST | Exception Name                 |");
    trace("======================================================================================");

    for (size_t i = 0; i < 256; i++)
    {
        struct idt_entry *entry = &idt_descriptor[i];

        uint64_t offset = ((uint64_t)entry->off_high << 32) | ((uint64_t)entry->off_mid << 16) | entry->off_low;
        const char *exception_name = (i < 32) ? strings[i] : "IRQ/Custom";

        trace("| %3lu | 0x%016llx | 0x%04x    | 0x%02x |  %d  | %-30s |",
              i, offset, entry->sel, entry->attr, entry->ist, exception_name);
    }

    trace("======================================================================================");
}

void load_idt()
{
    __asm__ volatile(
        "lidt %0"
        : : "m"(idt_ptr) : "memory");
}

int idt_register_handler(size_t vector, idt_intr_handler handler)
{
    if (real_handlers[vector] == idt_default_interrupt_handler)
    {
        real_handlers[vector] = handler;
        return 0;
    }
    return 1;
}