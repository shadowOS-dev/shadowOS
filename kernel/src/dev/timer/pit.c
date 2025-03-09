#include <dev/timer/pit.h>
#include <dev/portio.h>
#include <sys/pic.h>
#include <sys/idt.h>
#include <lib/log.h>

#define PIT0_DATA 0x40
#define PIT_CMD 0x43

void pit_handler(int_frame_t *frame)
{
    (void)frame;
    debug("tick");
}

void pit_init()
{
    // Setup channel 0 at mode 3 (lohi)
    outb(PIT_CMD, 0x36);

    // Register our IRQ0 handller (aka the pit handler)
    IDT_REGISTER_IRQ_HANDLER(0, pit_handler);

    // Setup the divisor
    uint16_t divisor = 5966; // ~200Hz
    outb(PIT0_DATA, divisor & 0xFF);
    outb(PIT0_DATA, (divisor >> 8) & 0xFF);

    // unmask the interrupt
    pic_unmask(0);
}