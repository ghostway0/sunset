#pragma once

#include <stddef.h>

struct point {
    size_t x;
    size_t y;
};

struct rect {
    struct point pos;
    int w;
    int h;
};

struct image {
    size_t w;
    size_t h;
    struct color *pixels;
};

void show_image_grayscale(struct image const *image);

void show_image_grayscale_at(struct image const *image, struct point pos);
