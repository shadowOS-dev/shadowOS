#include <sys/pic.h>
#include <dev/portio.h>
#include <lib/log.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

enum
{
    PIC_ICW1_ICW4 = 0x01,
    PIC_ICW1_SINGLE = 0x02,
    PIC_ICW1_INTERVAL4 = 0x04,
    PIC_ICW1_LEVEL = 0x08,
    PIC_ICW1_INITIALIZE = 0x10
} PIC_ICW1;

enum
{
    PIC_ICW4_8086 = 0x1,
    PIC_ICW4_AUTO_EOI = 0x2,
    PIC_ICW4_BUFFER_MASTER = 0x4,
    PIC_ICW4_BUFFER_SLAVE = 0x0,
    PIC_ICW4_BUFFERRED = 0x8,
    PIC_ICW4_SFNM = 0x10,
} PIC_ICW4;

enum
{
    PIC_CMD_END_OF_INTERRUPT = 0x20,
    PIC_CMD_READ_IRR = 0x0A,
    PIC_CMD_READ_ISR = 0x0B,
} PIC_CMD;

static uint16_t g_picmask = 0xffff;

void pic_setmask(uint16_t newMask)
{
    g_picmask = newMask;
    outb(PIC1_DATA, g_picmask & 0xFF);
    io_wait();
    outb(PIC2_DATA, g_picmask >> 8);
    io_wait();
    trace("Set mask to 0x%08llX", g_picmask);
}

uint16_t pic_getmask()
{
    uint16_t mask = inb(PIC1_DATA) | (inb(PIC2_DATA) << 8);
    trace("Get mask: 0x%08llX", mask);
    return mask;
}

void pic_configure(uint8_t offset_pic1, uint8_t offset_pic2, bool auto_eoi)
{
    trace("Configuring PIC with offsets 0x%04llX and 0x%04llX, AutoEOI: %s", offset_pic1, offset_pic2, auto_eoi ? "true" : "false");
    pic_setmask(0xFFFF);

    outb(PIC1_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();
    outb(PIC2_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();

    outb(PIC1_DATA, offset_pic1);
    io_wait();
    outb(PIC2_DATA, offset_pic2);
    io_wait();

    outb(PIC1_DATA, 0x4);
    io_wait();
    outb(PIC2_DATA, 0x2);
    io_wait();

    uint8_t icw4 = PIC_ICW4_8086;
    if (auto_eoi)
    {
        icw4 |= PIC_ICW4_AUTO_EOI;
    }

    outb(PIC1_DATA, icw4);
    io_wait();
    outb(PIC2_DATA, icw4);
    io_wait();

    pic_setmask(0xFFFF);
}

void pic_sendendofinterrupt(int irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_CMD_END_OF_INTERRUPT);
    outb(PIC1_COMMAND, PIC_CMD_END_OF_INTERRUPT);
}

void pic_disable()
{
    trace("Disabling PIC");
    pic_setmask(0xFFFF);
}

void pic_enable()
{
    trace("Enabling PIC");
    outb(PIC1_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();
    outb(PIC2_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();

    outb(PIC1_DATA, PIC_REMAP_OFFSET);
    io_wait();
    outb(PIC2_DATA, PIC_REMAP_OFFSET + 8);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_wait();

    uint8_t mask1 = inb(PIC1_DATA) & ~(1 << 2);
    outb(PIC1_DATA, mask1);
    io_wait();
    uint8_t mask2 = inb(PIC2_DATA) & ~0x80;
    outb(PIC2_DATA, mask2);
    io_wait();
}

void pic_mask(int irq)
{
    trace("Masking IRQ %d", irq);
    pic_setmask(g_picmask | (1 << irq));
}

void pic_unmask(int irq)
{
    trace("Unmasking IRQ %d", irq);
    pic_setmask(g_picmask & ~(1 << irq));
}

uint16_t pic_readirqrequestregister()
{
    trace("Reading IRQ Request Register");
    outb(PIC1_COMMAND, PIC_CMD_READ_IRR);
    outb(PIC2_COMMAND, PIC_CMD_READ_IRR);
    return ((uint16_t)inb(PIC2_COMMAND)) | (((uint16_t)inb(PIC2_COMMAND)) << 8);
}

uint16_t pic_readinserviceregister()
{
    trace("Reading In-Service Register");
    outb(PIC1_COMMAND, PIC_CMD_READ_ISR);
    outb(PIC2_COMMAND, PIC_CMD_READ_ISR);
    return ((uint16_t)inb(PIC2_COMMAND)) | (((uint16_t)inb(PIC2_COMMAND)) << 8);
}
