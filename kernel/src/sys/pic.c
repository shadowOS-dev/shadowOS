#include <sys/pic.h>
#include <dev/portio.h>
#include <lib/log.h>

// Credits to: https://github.com/FrostyOS-dev/FrostyOS/, i swear if this doesnt work i give up.
#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1

#define PIC_EOI 0x20

#define PIC_ICW1_ICW4 0x01
#define PIC_ICW1_SINGLE 0x02
#define PIC_ICW1_INTERVAL4 0x04
#define PIC_ICW1_LEVEL 0x08
#define PIC_ICW1_INIT 0x10

#define PIC_ICW4_8086 0x01
#define PIC_ICW4_AUTO 0x02
#define PIC_ICW4_BUF_SLAVE 0x08
#define PIC_ICW4_BUF_MASTER 0x0C
#define PIC_ICW4_SFNM 0x10

uint16_t g_curmask = 0xFFFF;

// PIC Initialization
void pic_init()
{
    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    trace("Started initialization sequence in cascade mode");

    outb(PIC_MASTER_DATA, 0x20); // 0x20 = Remap offset
    io_wait();
    outb(PIC_SLAVE_DATA, 0x28); // Remap offset + 8
    io_wait();

    outb(PIC_MASTER_DATA, 4); // Tell master PIC there is a slave at IRQ2
    io_wait();
    outb(PIC_SLAVE_DATA, 2); // Tell slave PIC its cascade identity
    io_wait();

    outb(PIC_MASTER_DATA, PIC_ICW4_8086); // Set master to 8086 mode
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086); // Set slave to 8086 mode
    io_wait();

    // Zero out the data for slave and master
    outb(PIC_MASTER_DATA, 0);
    outb(PIC_SLAVE_DATA, 0);
    trace("Unmasked master and slave");

    pic_maskall(); // Mask all IRQs initially
    trace("Masked all IRQs initially");
}

// End of interrupt handling
void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC_SLAVE_COMMAND, PIC_EOI); // Send EOI to the slave PIC
        trace("Sent EOI to slave PIC for IRQ %d", irq);
    }
    outb(PIC_MASTER_COMMAND, PIC_EOI); // Send EOI to the master PIC
    trace("Sent EOI to master PIC for IRQ %d", irq);
}

// Mask a specific IRQ
void pic_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC_MASTER_DATA;
    }
    else
    {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
    trace("Masked IRQ %d", irq);
}

// Unmask a specific IRQ
void pic_unmask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC_MASTER_DATA;
    }
    else
    {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
    trace("Unmasked IRQ %d", irq);
}

// Mask all IRQs
void pic_maskall()
{
    outb(PIC_MASTER_DATA, 0xFF); // Mask all IRQs on the master
    outb(PIC_SLAVE_DATA, 0xFF);  // Mask all IRQs on the slave
    g_curmask = 0xFFFF;
    trace("Masked all IRQs");
}
