#pragma once

#include <stddef.h>

#include "sunset/geometry.h"

#define COLOR_WHITE color_from_rgb(255, 255, 255)
#define COLOR_BLACK color_from_rgb(0, 0, 0)
#define COLOR_TRANSPARENT (Color){0}

enum image_format {
    IMAGE_FORMAT_GRAY,
    IMAGE_FORMAT_RGB,
    IMAGE_FORMAT_RGBA,
};

typedef struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

Color color_from_rgb(uint8_t r, uint8_t g, uint8_t b);

Color color_from_16bit(uint16_t color);

Color color_from_grayscale(uint8_t value);

Color color_from_hex(char const *hex_str);

bool colors_equal(Color a, Color b);

uint8_t color_to_grayscale(Color color);

// TODO: multiformat
typedef struct Image {
    size_t w;
    size_t h;
    Color *pixels;
} Image;

// void image_convert(Image const *image, enum image_format
// targeT_format, Image *image_out);

void image_destroy(Image *image);

int load_image_file(char const *path, Image *image_out);

void show_image_grayscale(Image const *image);

void show_image_grayscale_at(Image const *image, Point pos);
