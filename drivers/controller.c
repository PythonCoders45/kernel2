/* ==========================================================================
 * OS Input Subsystem - Standard Gamepad & Controller Driver
 * ========================================================================== */

#include "../include/types.h"
#include "../include/input.h"

// Controller Button Flags
#define BTN_A      0x0001
#define BTN_B      0x0002
#define BTN_START  0x0004
#define BTN_DPAD_U 0x0008

int controller_poll_state(uint32_t raw_button_mask, int32_t stick_x, int32_t stick_y, InputEvent* out_event) {
    out_event->device_type = INPUT_DEV_CONTROLLER;
    out_event->event_id = raw_button_mask;
    out_event->data_x = stick_x; // Normalized analog stick X (-100 to 100)
    out_event->data_y = stick_y; // Normalized analog stick Y (-100 to 100)
    out_event->is_pressed = (raw_button_mask != 0);

    return 1;
}
