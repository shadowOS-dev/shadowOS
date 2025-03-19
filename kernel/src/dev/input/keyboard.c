#include <dev/input/keyboard.h>
#include <dev/portio.h>
#include <lib/log.h>
#include <sys/intr.h>
#include <sys/pic.h>
#include <fs/devfs.h>

static volatile uint8_t last_scancode = 0;
static volatile uint8_t has_scancode = 0;

static inline void wait_for_write(void)
{
    while (inb(PS2_IO_STATUS) & PS2_STATUS_INPUT_FULL)
        ;
}

static inline void wait_for_read(void)
{
    while (!(inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL))
        ;
}

static void kbd_send_command(uint8_t command)
{
    wait_for_write();
    outb(PS2_IO_DATA, command);
}

uint8_t kbd_read_scancode(void)
{
    wait_for_read();
    return inb(PS2_IO_DATA);
}

// Keyboard interrupt handler (hooked into IRQ1)
void kbd_handler(struct register_ctx *)
{
    uint8_t scancode = kbd_read_scancode();

    if (scancode == KBD_ACK)
    {
        last_scancode = 0;
        has_scancode = 0;
    }
    else
    {
        last_scancode = scancode;
        has_scancode = 1;
    }

    pic_eoi(1);
}

// Initialize the keyboard
void kbd_init(void)
{
    trace("Initializing PS/2 keyboard");

    // Enable the first PS/2 port (keyboard), if not already enabled
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_ENABLE_FIRST_PORT);

    // Reset the keyboard
    kbd_send_command(KBD_CMD_RESET);
    uint8_t response = kbd_read_scancode();

    if (response == KBD_ACK)
    {
        trace("Keyboard reset successful");
    }
    else
    {
        warning("Keyboard reset failed, response: 0x%02X", response);
    }

    // Enable scanning
    kbd_send_command(KBD_CMD_ENABLE_SCANNING);

    // Enable IRQ1
    idt_register_handler(IDT_IRQ_BASE + 1, kbd_handler);
    pic_unmask(1);
}

// ---- HANDLE /dev/ps2kb* ----
int kbd_read(void *out, size_t size, size_t)
{
    if (has_scancode)
    {
        if (size >= sizeof(last_scancode))
        {
            *(uint8_t *)out = last_scancode;
            has_scancode = 0;
        }
        return sizeof(last_scancode);
    }
    else
    {
        return -1;
    }
}

// Unused
int kbd_write(const void *, size_t, size_t)
{
    return -1;
}

void kbd_register_fs(const char *name)
{
    devfs_add_dev(name, kbd_read, kbd_write);
}
