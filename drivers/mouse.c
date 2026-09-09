/* ==========================================================================
 * OS Input Subsystem - Mouse & Pointer Driver
 * ========================================================================== */

#include "../include/types.h"
#include "../include/input.h"

static int32_t cursor_x = 320;
static int32_t cursor_y = 240;

// Processes standard 3-byte PS/2 mouse packets
int mouse_process_packet(uint8_t b1, uint8_t b2, uint8_t b3, InputEvent* out_event) {
    // Check synchronization bit
    if (!(b1 & 0x08)) return 0;

    int dx = (int8_t)b2;
    int dy = (int8_t)b3; // PS/2 Y is inverted

    cursor_x += dx;
    cursor_y -= dy;

    // Screen bounds clamping (assuming 640x480)
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x > 640) cursor_x = 640;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y > 480) cursor_y = 480;

    out_event->device_type = INPUT_DEV_MOUSE;
    out_event->event_id = b1 & 0x07; // Button state bitmask (Left, Right, Middle)
    out_event->data_x = cursor_x;
    out_event->data_y = cursor_y;
    out_event->is_pressed = (b1 & 0x01); // Primary click state

    return 1;
}
