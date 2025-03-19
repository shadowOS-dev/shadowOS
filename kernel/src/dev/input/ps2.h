#ifndef DEV_INPUT_PS2_H
#define DEV_INPUT_PS2_H

#define PS2_IO_DATA 0x60             // Read/Write
#define PS2_IO_STATUS 0x64           // Read
#define PS2_IO_COMMAND PS2_IO_STATUS // Write

// Status Register Bits (0x64 Read)
#define PS2_STATUS_OUTPUT_FULL (1 << 0)  // Output buffer full (data available)
#define PS2_STATUS_INPUT_FULL (1 << 1)   // Input buffer full (controller busy)
#define PS2_STATUS_SYSTEM_FLAG (1 << 2)  // System flag (set after self-test)
#define PS2_STATUS_COMMAND_DATA (1 << 3) // 0 = Data from device, 1 = Data from controller
#define PS2_STATUS_TIMEOUT_ERR (1 << 6)  // Timeout error
#define PS2_STATUS_PARITY_ERR (1 << 7)   // Parity error

// Controller Commands (0x64 Write)
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_SECOND_PORT 0xA7
#define PS2_CMD_ENABLE_SECOND_PORT 0xA8
#define PS2_CMD_TEST_SECOND_PORT 0xA9
#define PS2_CMD_TEST_CONTROLLER 0xAA
#define PS2_CMD_TEST_FIRST_PORT 0xAB
#define PS2_CMD_DISABLE_FIRST_PORT 0xAD
#define PS2_CMD_ENABLE_FIRST_PORT 0xAE
#define PS2_CMD_READ_CONTROLLER_OUT 0xD0
#define PS2_CMD_WRITE_CONTROLLER_OUT 0xD1
#define PS2_CMD_WRITE_SECOND_PORT 0xD4 // Send byte to second PS/2 port

// Configuration Byte Bits (read/write using 0x20/0x60)
#define PS2_CONFIG_FIRST_PORT_INT (1 << 0)  // First port interrupt enable
#define PS2_CONFIG_SECOND_PORT_INT (1 << 1) // Second port interrupt enable
#define PS2_CONFIG_SYSTEM_FLAG (1 << 2)     // Must be 1 (self-test passed)
#define PS2_CONFIG_FIRST_PORT_CLK (1 << 4)  // 0 = Enabled, 1 = Disabled
#define PS2_CONFIG_SECOND_PORT_CLK (1 << 5) // 0 = Enabled, 1 = Disabled
#define PS2_CONFIG_FIRST_TRANSLATE (1 << 6) // First port translation enable

void ps2_init();

#endif // DEV_INPUT_PS2_H
