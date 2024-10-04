#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

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

struct box {
    vec3 min;
    vec3 max;
};

struct texture {
    uint32_t id;
    size_t width;
    size_t height;
    size_t channels;
    char const *data;
    size_t data_size;
};

struct mesh {
    vec3 *vertices;
    size_t num_vertices;
    uint32_t *indices;
    size_t num_indices;
    struct effect *effects;
    size_t num_effects;
};

void show_image_grayscale(struct image const *image);

void show_image_grayscale_at(struct image const *image, struct point pos);
