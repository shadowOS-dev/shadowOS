#include <dev/timer/pit.h>
#include <dev/portio.h>
#include <sys/pic.h>
#include <lib/log.h>
#include <sys/intr.h>

void pit_handler(struct register_ctx *frame)
{
    (void)frame;
    debug("tick");
}

void pit_init()
{
    // Setup channel 0 at mode 3 (lohi)
    outb(0x43, 0x36);
    trace("Setup chanel 0 to mode 3 (LOHI)");

    // Register our IRQ0 handller (aka the pit handler)
    idt_register_handler(IDT_IRQ_BASE + 0, pit_handler);

    // Setup the divisor
    uint16_t divisor = 5966; // ~200Hz
    outb(0x40, divisor & 0xFF);
    trace("Devisor thing 1");
    outb(0x40, (divisor >> 8) & 0xFF);
    trace("Devisor thing 2");
    trace("Set divisor to %d", divisor);

    // unmask the IRQ0
    // pic_unmask(0);
}