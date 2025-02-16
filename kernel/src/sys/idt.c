#include <sys/idt.h>
#include <lib/log.h>
#include <util/cpu.h>

idt_ptr_t idt_ptr;
idt_entry_t idt_entries[IDT_ENTRY_COUNT];
extern uint64_t isr_table[];

void idt_set_gate(idt_entry_t idt[], int num, uint64_t base, uint16_t segment, uint8_t flags)
{
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_middle = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].segment_selector = segment;
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

void idt_init()
{
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRY_COUNT - 1;
    idt_ptr.base = (uint64_t)&idt_entries;

    for (int i = 0; i < 32; i++)
    {
        idt_set_gate(idt_entries, i, isr_table[i], 0x08, 0x8E);
    }

    idt_load((uint64_t)&idt_ptr);
}

static const char *exception_mnemonics[] = {
    "#DE", "#DB", "--", "#BP", "#OF", "#BR", "#UD", "#NM",
    "#DF", "--", "#TS", "#NP", "#SS", "#GP", "#PF", "--",
    "#MF", "#AC", "#MC", "#XF", "--", "--", "--", "--",
    "--", "--", "--", "--", "--", "--", "--", "--"};

void idt_handler(int_frame_t frame)
{
    if (frame.vector < 32)
    {
        const char *mnemonic = exception_mnemonics[frame.vector];
        error("Exception: %s (0x%x) at RIP: 0x%.16llx", mnemonic, frame.vector, frame.rip);
        error(" Error Code: 0x%llx", frame.err);

        error("Register Dump:");
        error(" RAX: 0x%016llx  RBX: 0x%016llx  RCX: 0x%016llx  RDX: 0x%016llx", frame.rax, frame.rbx, frame.rcx, frame.rdx);
        error(" RSI: 0x%016llx  RDI: 0x%016llx  RBP: 0x%016llx  RSP: 0x%016llx", frame.rsi, frame.rdi, frame.rbp, frame.rsp);
        error(" R8 : 0x%016llx  R9 : 0x%016llx  R10: 0x%016llx  R11: 0x%016llx", frame.r8, frame.r9, frame.r10, frame.r11);
        error(" R12: 0x%016llx  R13: 0x%016llx  R14: 0x%016llx  R15: 0x%016llx", frame.r12, frame.r13, frame.r14, frame.r15);
        error(" CR2: 0x%016llx  CR3: 0x%016llx", frame.cr2, frame.cr3);
        error(" CS : 0x%016llx  SS : 0x%016llx  RFLAGS: 0x%016llx", frame.cs, frame.ss, frame.rflags);

        hcf();
    }
    else if (frame.vector == 0x80)
    {
        error("Dropped system call %d.", frame.rax);
    }
    else
    {
        error("Unhandled interrupt: 0x%x at RIP: 0x%.16llx", frame.vector, frame.rip);
    }
}
