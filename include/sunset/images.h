#pragma once

#include <stddef.h>

#include "sunset/geometry.h"

struct image {
    size_t w;
    size_t h;
    struct color *pixels;
};

void image_destroy(struct image *image);

int load_image_file(char const *path, struct image *image_out);

void show_image_grayscale(struct image const *image);

void show_image_grayscale_at(struct image const *image, struct point pos);
