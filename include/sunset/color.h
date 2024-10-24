#pragma once

#include <stdbool.h>
#include <stdint.h>

#define COLOR_WHITE color_from_rgb(255, 255, 255)
#define COLOR_BLACK color_from_rgb(0, 0, 0)

struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct color color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

struct color color_from_hex(char const *hex_str);

struct color color_from_grayscale(uint8_t value);

bool colors_equal(struct color a, struct color b);

uint8_t color_to_grayscale(struct color color);
