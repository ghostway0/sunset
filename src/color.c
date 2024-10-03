#include <stdio.h>

#include "sunset/color.h"

struct color color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
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

