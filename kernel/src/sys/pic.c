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

void pic_init()
{
    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    trace("Sent init shit to slave and master");

    outb(PIC_MASTER_DATA, 0x20); // 0x20=Remap offset
    io_wait();
    outb(PIC_SLAVE_DATA, 0x28); // remap offset + 8
    io_wait();

    outb(PIC_MASTER_DATA, 4);
    io_wait();
    outb(PIC_SLAVE_DATA, 2);
    io_wait();

    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    pic_maskall();
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

void pic_mask(uint8_t irq)
{
    g_curmask |= (1 << irq);
    if (irq < 8)
        outb(PIC_MASTER_DATA, g_curmask & 0xFF);
    else
        outb(PIC_SLAVE_DATA, (g_curmask >> 8) & 0xFF);
}

void pic_unmask(uint8_t irq)
{
    g_curmask &= ~(1 << irq);
    if (irq < 8)
        outb(PIC_MASTER_DATA, g_curmask & 0xFF);
    else
        outb(PIC_SLAVE_DATA, (g_curmask >> 8) & 0xFF);
}

void pic_maskall()
{
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA, 0xFF);
    g_curmask = 0xFFFF;
}