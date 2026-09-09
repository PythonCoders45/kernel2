#ifndef INPUT_H
#define INPUT_H

#include "types.h"

// Device types generating the event
typedef enum {
    INPUT_DEV_KEYBOARD = 0,
    INPUT_DEV_MOUSE,
    INPUT_DEV_CONTROLLER,
    INPUT_DEV_REMOTE
} InputDeviceType;

// Standardized Event Structure
typedef struct {
    InputDeviceType device_type;
    uint32_t event_id;       // Keycode, button ID, or control flag
    int32_t  data_x;         // Mouse delta X or analog stick X
    int32_t  data_y;         // Mouse delta Y or analog stick Y
    uint8_t  is_pressed;     // 1 for down/click, 0 for release
} InputEvent;

#endif
