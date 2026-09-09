/* ==========================================================================
 * OS Image Subsystem - Block-Optimized Image Parser & Renderer
 * Loads assets by name via storage.c and draws using GUI.c primitives
 * ========================================================================== */

#include "../include/types.h"

/* ==========================================================================
 * 1. EXTERNAL DEPENDENCIES (Linked from storage.c and GUI.c)
 * ========================================================================== */

extern int storage_load_file_to_ram(const char* filename, uint8_t* ram_destination);
extern void draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);

/* ==========================================================================
 * 2. STRUCTURE DEFINITIONS
 * ========================================================================== */

typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t bpp;
    uint32_t data_offset;
} BMPHeader;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t* raw_buffer;
} LoadedImage;

/* ==========================================================================
 * 3. BMP PARSING & HEADER EXTRACTION
 * ========================================================================== */

static BMPHeader image_parse_bmp_header(const uint8_t* file_buffer) {
    BMPHeader header;
    
    // Safely extract standard uncompressed BMP offsets without standard libraries
    header.width = *(uint32_t*)&file_buffer[18];
    header.height = *(uint32_t*)&file_buffer[22];
    header.bpp = *(uint16_t*)&file_buffer[28];
    header.data_offset = *(uint32_t*)&file_buffer[10];

    return header;
}

/* ==========================================================================
 * 4. PUBLIC ASSET LOADER API
 * ========================================================================== */

// Loads a BMP file from disk into a persistent RAM buffer by name
int image_load_asset(const char* filename, uint8_t* memory_pool, LoadedImage* out_img) {
    // Load file straight from storage sectors using our FFS driver
    if (storage_load_file_to_ram(filename, memory_pool) != 0) {
        return -1; // File load failed
    }

    BMPHeader header = image_parse_bmp_header(memory_pool);
    
    out_img->width = header.width;
    out_img->height = header.height;
    out_img->raw_buffer = memory_pool;

    return 0; // Success
}

/* ==========================================================================
 * 5. MATHEMATICAL PATTERN & BLOCK-OPTIMIZED RENDERER
 * ========================================================================== */

// Scans horizontal pixel rows for identical colors, compressing them into 
// single rectangle draw calls to maximize rendering performance.
void image_render_optimized(int32_t screen_x, int32_t screen_y, const LoadedImage* img) {
    BMPHeader header = image_parse_bmp_header(img->raw_buffer);
    const uint8_t* pixel_data = img->raw_buffer + header.data_offset;

    // BMP rows are padded to align on 4-byte boundaries
    uint32_t row_stride = ((header.width * (header.bpp / 8) + 3) & ~3);

    for (int32_t y = 0; y < (int32_t)header.height; y++) {
        // BMP stores image rows bottom-to-top; invert y to render correctly
        int32_t target_y = screen_y + (header.height - 1 - y);
        
        int32_t span_start_x = 0;
        uint32_t current_color = 0;
        uint32_t span_length = 0;
        int initialized = 0;

        for (int32_t x = 0; x < (int32_t)header.width; x++) {
            uint32_t index = (y * row_stride) + (x * (header.bpp / 8));
            
            uint8_t b = pixel_data[index];
            uint8_t g = pixel_data[index + 1];
            uint8_t r = pixel_data[index + 2];
            uint32_t rgb = (r << 16) | (g << 8) | b;

            if (!initialized) {
                current_color = rgb;
                span_start_x = x;
                span_length = 1;
                initialized = 1;
            } else if (rgb == current_color) {
                span_length++;
            } else {
                // Color changed—flush the accumulated horizontal line block instantly
                if (span_length > 0) {
                    draw_rect(screen_x + span_start_x, target_y, span_length, 1, current_color);
                }
                // Reset tracker for the next color span
                current_color = rgb;
                span_start_x = x;
                span_length = 1;
            }
        }

        // Flush any remaining pixel span at the end of the row
        if (span_length > 0) {
            draw_rect(screen_x + span_start_x, target_y, span_length, 1, current_color);
        }
    }
}
