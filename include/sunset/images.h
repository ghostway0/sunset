#pragma once

#include <stddef.h>

#include "sunset/geometry.h"

#define COLOR_WHITE color_from_rgb(255, 255, 255)
#define COLOR_BLACK color_from_rgb(0, 0, 0)

enum image_format {
    IMAGE_FORMAT_GRAY,
    IMAGE_FORMAT_RGB,
    IMAGE_FORMAT_RGBA,
};

struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct color color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

struct color color_from_16bit(uint16_t color);

struct color color_from_grayscale(uint8_t value);

struct color color_from_hex(char const *hex_str);

bool colors_equal(struct color a, struct color b);

uint8_t color_to_grayscale(struct color color);

struct image {
    size_t w;
    size_t h;
    struct color *pixels;
};

// void image_convert(struct image const *image, struct image *image_out);

void image_destroy(struct image *image);

int load_image_file(char const *path, struct image *image_out);

void show_image_grayscale(struct image const *image);

void show_image_grayscale_at(struct image const *image, struct point pos);
