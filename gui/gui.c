#ifndef TYPES_H
#define TYPES_H

// Custom fixed-width types (replacing standard headers for freestanding OS)
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef uint32_t size_t;

#endif

// --- CORE SCREEN STATE ---

typedef struct {
    uint32_t* framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch; // Bytes per scanline
} Screen;

static Screen current_screen;

void gui_init(uint32_t* fb, uint32_t width, uint32_t height, uint32_t pitch) {
    current_screen.framebuffer = fb;
    current_screen.width = width;
    current_screen.height = height;
    current_screen.pitch = pitch;
}

// --- COLOR MATH: ALPHA BLENDING & HSV ---

uint32_t blend_colors(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;

    uint32_t r_bg = (bg >> 16) & 0xFF;
    uint32_t g_bg = (bg >> 8) & 0xFF;
    uint32_t b_bg = bg & 0xFF;

    uint32_t r_fg = (fg >> 16) & 0xFF;
    uint32_t g_fg = (fg >> 8) & 0xFF;
    uint32_t b_fg = fg & 0xFF;

    uint32_t r = (r_fg * alpha + r_bg * (255 - alpha)) / 255;
    uint32_t g = (g_fg * alpha + g_bg * (255 - alpha)) / 255;
    uint32_t b = (b_fg * alpha + b_bg * (255 - alpha)) / 255;

    return (r << 16) | (g << 8) | b;
}

uint32_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        uint8_t brightness = (v * 255) / 100;
        return (brightness << 16) | (brightness << 8) | brightness;
    }

    region = h / 60;
    remainder = (h - (region * 60)) * 4; // approximate scale

    p = (v * (100 - s) * 255) / 10000;
    q = (v * (100 - (s * remainder) / 255) * 255) / 10000;
    t = (v * (100 - (s * (255 - remainder)) / 255) * 255) / 10000;
    
    uint8_t r_val = (v * 255) / 100;
    uint8_t g_val = r_val;
    uint8_t b_val = r_val;

    switch (region) {
        case 0:  r_val = (v*255)/100; g_val = t; b_val = p; break;
        case 1:  r_val = q; g_val = (v*255)/100; b_val = p; break;
        case 2:  r_val = p; g_val = (v*255)/100; b_val = t; break;
        case 3:  r_val = p; g_val = q; b_val = (v*255)/100; break;
        case 4:  r_val = t; g_val = p; b_val = (v*255)/100; break;
        default: r_val = (v*255)/100; g_val = p; b_val = q; break;
    }

    return (r_val << 16) | (g_val << 8) | b_val;
}

// --- PRIMITIVE: PIXEL & ALPHA PIXEL ---

void draw_pixel(int32_t x, int32_t y, uint32_t color) {
    if (x < 0 || x >= (int32_t)current_screen.width || y < 0 || y >= (int32_t)current_screen.height) {
        return;
    }
    uint32_t* pixel_address = (uint32_t*)((uint8_t*)current_screen.framebuffer + y * current_screen.pitch) + x;
    *pixel_address = color;
}

void draw_pixel_alpha(int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
    if (x < 0 || x >= (int32_t)current_screen.width || y < 0 || y >= (int32_t)current_screen.height) {
        return;
    }
    uint32_t* pixel_address = (uint32_t*)((uint8_t*)current_screen.framebuffer + y * current_screen.pitch) + x;
    
    if (alpha < 255) {
        *pixel_address = blend_colors(*pixel_address, color, alpha);
    } else {
        *pixel_address = color;
    }
}

// --- PRIMITIVE: LINE (Bresenham's Algorithm) ---

void draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    int32_t dx = __builtin_abs(x1 - x0);
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t dy = -__builtin_abs(y1 - y0);
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    int32_t e2;

    while (1) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// --- SHAPES ---

void draw_quad(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color, uint8_t filled) {
    if (filled) {
        for (int32_t y = y0; y <= y1; y++) {
            for (int32_t x = x0; x <= x1; x++) {
                draw_pixel(x, y, color);
            }
        }
    } else {
        draw_line(x0, y0, x1, y0, color);
        draw_line(x1, y0, x1, y1, color);
        draw_line(x0, y1, x1, y1, color);
        draw_line(x0, y0, x0, y1, color);
    }
}

void draw_circle(int32_t xc, int32_t yc, int32_t r, uint32_t color, uint8_t filled) {
    int32_t x = 0;
    int32_t y = r;
    int32_t d = 3 - 2 * r;

    while (y >= x) {
        if (filled) {
            draw_line(xc - x, yc + y, xc + x, yc + y, color);
            draw_line(xc - x, yc - y, xc + x, yc - y, color);
            draw_line(xc - y, yc + x, xc + y, yc + x, color);
            draw_line(xc - y, yc - x, xc + y, yc - x, color);
        } else {
            draw_pixel(xc + x, yc + y, color);
            draw_pixel(xc - x, yc + y, color);
            draw_pixel(xc + x, yc - y, color);
            draw_pixel(xc - x, yc - y, color);
            draw_pixel(xc + y, yc + x, color);
            draw_pixel(xc - y, yc + x, color);
            draw_pixel(xc + y, yc - x, color);
            draw_pixel(xc - y, yc - x, color);
        }
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_triangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, uint8_t filled) {
    if (!filled) {
        draw_line(x0, y0, x1, y1, color);
        draw_line(x1, y1, x2, y2, color);
        draw_line(x2, y2, x0, y0, color);
    } else {
        int32_t min_x = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int32_t max_x = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        int32_t min_y = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
        int32_t max_y = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

        for (int32_t y = min_y; y <= max_y; y++) {
            for (int32_t x = min_x; x <= max_x; x++) {
                int32_t s = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
                int32_t t = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
                int32_t d = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);

                if ((s >= 0 && t >= 0 && d >= 0) || (s <= 0 && t <= 0 && d <= 0)) {
                    draw_pixel(x, y, color);
                }
            }
        }
    }
}

void draw_half_circle(int32_t xc, int32_t yc, int32_t r, uint32_t color, uint8_t filled, uint8_t mode) {
    int32_t x = 0;
    int32_t y = r;
    int32_t d = 3 - 2 * r;

    while (y >= x) {
        if (mode == 0) {
            if (filled) {
                draw_line(xc - x, yc - y, xc + x, yc - y, color);
                draw_line(xc - y, yc - x, xc + y, yc - x, color);
            } else {
                draw_pixel(xc + x, yc - y, color);
                draw_pixel(xc - x, yc - y, color);
                draw_pixel(xc + y, yc - x, color);
                draw_pixel(xc - y, yc - x, color);
            }
        } else {
            if (filled) {
                draw_line(xc - x, yc + y, xc + x, yc + y, color);
                draw_line(xc - y, yc + x, xc + y, yc + x, color);
            } else {
                draw_pixel(xc + x, yc + y, color);
                draw_pixel(xc - x, yc + y, color);
                draw_pixel(xc + y, yc + x, color);
                draw_pixel(xc - y, yc + x, color);
            }
        }
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_ellipse(int32_t xc, int32_t yc, int32_t rx, int32_t ry, uint32_t color, uint8_t filled) {
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t two_rx2 = 2 * rx2;
    int32_t two_ry2 = 2 * ry2;
    int32_t p;
    int32_t x = 0;
    int32_t y = ry;
    int32_t px = 0;
    int32_t py = two_rx2 * y;

    p = (int32_t)(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        if (filled) {
            draw_line(xc - x, yc + y, xc + x, yc + y, color);
            draw_line(xc - x, yc - y, xc + x, yc - y, color);
        } else {
            draw_pixel(xc + x, yc + y, color);
            draw_pixel(xc - x, yc + y, color);
            draw_pixel(xc + x, yc - y, color);
            draw_pixel(xc - x, yc - y, color);
        }
        x++;
        px += two_ry2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= two_rx2;
            p += ry2 + px - py;
        }
    }

    p = (int32_t)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        if (filled) {
            draw_line(xc - x, yc + y, xc + x, yc + y, color);
            draw_line(xc - x, yc - y, xc + x, yc - y, color);
        } else {
            draw_pixel(xc + x, yc + y, color);
            draw_pixel(xc - x, yc + y, color);
            draw_pixel(xc + x, yc - y, color);
            draw_pixel(xc - x, yc - y, color);
        }
        y--;
        py -= two_rx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += two_ry2;
            p += rx2 - py + px;
        }
    }
}

// --- GENERAL POLYGON ---

typedef struct {
    int32_t x;
    int32_t y;
} Point;

void draw_polygon(Point* points, int32_t num_points, uint32_t color, uint8_t filled) {
    if (num_points < 3) return;

    for (int32_t i = 0; i < num_points; i++) {
        int32_t next = (i + 1) % num_points;
        draw_line(points[i].x, points[i].y, points[next].x, points[next].y, color);
    }

    if (filled) {
        int32_t min_x = points[0].x, max_x = points[0].x;
        int32_t min_y = points[0].y, max_y = points[0].y;

        for (int32_t i = 1; i < num_points; i++) {
            if (points[i].x < min_x) min_x = points[i].x;
            if (points[i].x > max_x) max_x = points[i].x;
            if (points[i].y < min_y) min_y = points[i].y;
            if (points[i].y > max_y) max_y = points[i].y;
        }

        for (int32_t y = min_y; y <= max_y; y++) {
            for (int32_t x = min_x; x <= max_x; x++) {
                int32_t odd_nodes = 0;
                int32_t j = num_points - 1;
                
                for (int32_t i = 0; i < num_points; i++) {
                    if ((points[i].y < y && points[j].y >= y) || (points[j].y < y && points[i].y >= y)) {
                        if (points[i].x + (float)(y - points[i].y) / (points[j].y - points[i].y) * (points[j].x - points[i].x) < x) {
                            odd_nodes = !odd_nodes;
                        }
                    }
                    j = i;
                }

                if (odd_nodes) {
                    draw_pixel(x, y, color);
                }
            }
        }
    }
}

// --- SHADOW & LIGHTING ENGINE ---

void draw_drop_shadow(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t blur_offset) {
    // Renders a soft-fading dark shadow layer beneath windows/elements
    for (int32_t i = 1; i <= blur_offset; i++) {
        uint8_t alpha = (uint8_t)(40 / i); // Fades out smoothly
        // Draw expanded box with alpha blending for shadow effect
        for (int32_t y = y0 + i; y <= y1 + i; y++) {
            for (int32_t x = x0 + i; x <= x1 + i; x++) {
                draw_pixel_alpha(x, y, 0x000000, alpha);
            }
        }
    }
}

// --- GUI WINDOW MANAGEMENT ---

typedef struct {
    int32_t x, y, width, height;
    uint32_t border_color;
    uint32_t bg_color;
    uint8_t active;
} Window;

void draw_window(Window* win) {
    // 1. Draw dynamic lighting shadow first
    draw_drop_shadow(win->x, win->y, win->x + win->width, win->y + win->height, 5);

    // 2. Window body with transparency option
    draw_quad(win->x, win->y, win->x + win->width, win->y + win->height, win->bg_color, 1);
    
    // 3. Title bar
    draw_quad(win->x, win->y, win->x + win->width, win->y + 25, 0x000080, 1);
    
    // 4. Outer border
    draw_quad(win->x, win->y, win->x + win->width, win->y + win->height, win->border_color, 0);
}
