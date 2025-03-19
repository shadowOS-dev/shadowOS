#ifndef DEV_INPUT_KEYBOARD_H
#define DEV_INPUT_KEYBOARD_H

#include <dev/input/ps2.h>
#include <stdint.h>
#include <stdbool.h>

// PS/2 Keyboard Command Set
#define KBD_CMD_SET_LEDS 0xED
#define KBD_CMD_ECHO 0xEE
#define KBD_CMD_SCANCODE_SET 0xF0
#define KBD_CMD_IDENTIFY 0xF2
#define KBD_CMD_ENABLE_SCANNING 0xF4
#define KBD_CMD_DISABLE_SCANNING 0xF5
#define KBD_CMD_RESET 0xFF

// PS/2 Acknowledgment Responses
#define KBD_ACK 0xFA
#define KBD_RESEND 0xFE

void kbd_init(void);
void kbd_register_fs(const char *name);

#endif // DEV_INPUT_KEYBOARD_H
