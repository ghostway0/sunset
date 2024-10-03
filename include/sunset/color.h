#pragma once

#include <stdint.h>

struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct color color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

struct color color_from_hex(char const *hex_str);
