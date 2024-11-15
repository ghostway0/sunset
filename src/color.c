#include <stdint.h>
#include <stdio.h>

#include "sunset/color.h"

struct color color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (struct color){r, g, b, 255};
}

struct color color_from_16bit(uint16_t color) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    return (struct color){r, g, b, 255};
}

struct color color_from_hex(char const *hex_str) {
    uint32_t hex;

    sscanf(hex_str, "#%x", &hex);
    return (struct color){
            .r = (hex >> 16) & 0xFF,
            .g = (hex >> 8) & 0xFF,
            .b = hex & 0xFF,
            .a = 255,
    };
}

bool colors_equal(struct color a, struct color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

uint8_t color_to_grayscale(struct color color) {
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

struct color color_from_grayscale(uint8_t value) {
    return (struct color){value, value, value, 255};
}
