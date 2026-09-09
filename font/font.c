/* ==========================================================================
 * OS Font Subsystem - Coordinate-Based UTF-8 Font Renderer
 * Parses multi-byte UTF-8 streams and draws glyphs at specific screen coords
 * ========================================================================== */

#include "../include/types.h"

extern int storage_load_file_to_ram(const char* filename, uint8_t* ram_destination);
extern void draw_pixel(int32_t x, int32_t y, uint32_t color);

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static uint8_t* font_glyph_data = 0;
static uint32_t font_loaded = 0;

int font_init(const char* filename, uint8_t* memory_pool) {
    if (storage_load_file_to_ram(filename, memory_pool) != 0) {
        return -1;
    }
    font_glyph_data = memory_pool;
    font_loaded = 1;
    return 0;
}

// Draw a single character glyph at exact (x, y) coordinates
void font_draw_char_at(int32_t x, int32_t y, uint32_t unicode_point, uint32_t color) {
    if (!font_loaded) return;

    // Fallback index for unsupported unicode points beyond basic ASCII block
    uint32_t glyph_index = (unicode_point < 256) ? unicode_point : '?';
    const uint8_t* glyph = font_glyph_data + (glyph_index * FONT_HEIGHT);

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (row_bits & (1 << (7 - col))) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

// Parse UTF-8 variable-length byte streams and render text strings at (x, y)
void font_draw_string(int32_t start_x, int32_t start_y, const char* utf8_str, uint32_t color) {
    int32_t current_x = start_x;
    int32_t current_y = start_y;
    const uint8_t* ptr = (const uint8_t*)utf8_str;

    while (*ptr) {
        uint32_t codepoint = 0;
        uint8_t byte1 = *ptr;

        if (byte1 < 0x80) {
            // 1-byte ASCII character (0xxxxxxx)
            codepoint = byte1;
            ptr += 1;
        } else if ((byte1 & 0xE0) == 0xC0) {
            // 2-byte UTF-8 character (110xxxxx 10xxxxxx)
            uint8_t byte2 = *(ptr + 1);
            codepoint = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
            ptr += 2;
        } else if ((byte1 & 0xF0) == 0xE0) {
            // 3-byte UTF-8 character (1110xxxx 10xxxxxx 10xxxxxx)
            uint8_t byte2 = *(ptr + 1);
            uint8_t byte3 = *(ptr + 2);
            codepoint = ((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
            ptr += 3;
        } else {
            // Skip invalid or 4-byte sequences for safety fallback
            codepoint = '?';
            ptr += 1;
        }

        if (codepoint == '\n') {
            current_x = start_x;
            current_y += FONT_HEIGHT + 4;
        } else {
            font_draw_char_at(current_x, current_y, codepoint, color);
            current_x += FONT_WIDTH + 2;
        }
    }
}
