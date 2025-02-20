#include <lib/debug.h>
#include <sys/idt.h>
#include <lib/log.h>
#include <util/cpu.h>

// TODO: Handle better
static void _int_handler(int_frame_t frame) {
    debug("DEBUG @ 0x%.16llx", frame.rip);
    hlt(); // halt cpu, not catch fire.
}

void debug_lib_init() {
    register_int_handler(0x01, _int_handler);
}
