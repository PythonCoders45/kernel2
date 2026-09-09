/* ==========================================================================
 * OS Input Subsystem - Keyboard Driver (x86 & PowerPC Multi-Arch)
 * ========================================================================== */

#include "../include/types.h"
#include "../include/input.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#if defined(__i386__) || defined(__x86_64__)
static inline uint8_t kb_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
#endif

// Simple US QWERTY scancode translation table (Set 1)
static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

int keyboard_poll_event(InputEvent* out_event) {
#if defined(__i386__) || defined(__x86_64__)
    uint8_t status = kb_inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) { // Output buffer full (key event ready)
        uint8_t scancode = kb_inb(KEYBOARD_DATA_PORT);
        
        out_event->device_type = INPUT_DEV_KEYBOARD;
        out_event->is_pressed = !(scancode & 0x80); // High bit set = release
        uint8_t clean_code = scancode & 0x7F;
        
        out_event->event_id = (clean_code < 128) ? scancode_ascii[clean_code] : 0;
        out_event->data_x = 0;
        out_event->data_y = 0;
        return 1; // Event captured
    }
#elif defined(__powerpc__)
    // Wii / PowerPC USB or GPIO keyboard polling stub
    // Reads from system input register buffers
    (void)out_event;
#endif
    return 0; // No event
}
