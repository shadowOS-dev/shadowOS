#include <dev/input/ps2.h>
#include <dev/portio.h>
#include <lib/log.h>

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

void ps2_init()
{
    // Disable the ports
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_DISABLE_FIRST_PORT);
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_DISABLE_SECOND_PORT);

    // Flush output buffer
    while (inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL)
    {
        inb(PS2_IO_DATA);
    }

    // Perform controller test
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_TEST_CONTROLLER);
    wait_for_read();
    uint8_t test_result = inb(PS2_IO_DATA);
    if (test_result != 0x55)
    {
        error("Failed to test PS/2 controller, returned 0x%02X", test_result);
        return;
    }
    trace("Successfully tested PS/2 controller");

    // Check for the second port
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_ENABLE_SECOND_PORT);
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_READ_CONFIG);
    wait_for_read();
    uint8_t conf = inb(PS2_IO_DATA);

    int has_second_port = (conf & PS2_CONFIG_SECOND_PORT_CLK) == 0;
    if (!has_second_port)
    {
        wait_for_write();
        outb(PS2_IO_COMMAND, PS2_CMD_DISABLE_SECOND_PORT);
        warning("There is no second PS/2 port");
    }
    else
    {
        trace("There is a second PS/2 port");
    }

    // Write the new config
    conf &= ~(PS2_CONFIG_FIRST_PORT_CLK | PS2_CONFIG_SECOND_PORT_CLK); // Enable ports
    conf |= PS2_CONFIG_FIRST_PORT_INT;                                 // Enable IRQ for first port
    if (has_second_port)
    {
        conf |= PS2_CONFIG_SECOND_PORT_INT; // Enable IRQ for second port, if present
    }

    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_WRITE_CONFIG);
    wait_for_write();
    outb(PS2_IO_DATA, conf);

    // Enable all ports (or those available)
    wait_for_write();
    outb(PS2_IO_COMMAND, PS2_CMD_ENABLE_FIRST_PORT);
    if (has_second_port)
    {
        wait_for_write();
        outb(PS2_IO_COMMAND, PS2_CMD_ENABLE_SECOND_PORT);
    }

    trace("Successfully initialized the PS/2 controller");
}
