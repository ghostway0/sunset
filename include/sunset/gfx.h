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

enum interpolation_type {
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBIC,
    INTERPOLATION_STEP,
};

struct keyframe {
    float time;
    vec3 position;
    vec3 rotation;
    float scale;
    enum interpolation_type position_interpolation;
    enum interpolation_type rotation_interpolation;
    enum interpolation_type scale_interpolation;
};

struct texture {
    uint32_t id;
    size_t width;
    size_t height;
    size_t channels;
    uint8_t *data;
    size_t data_size;
};

struct animation {
    char const *name;
    float duration;
    struct keyframe *keyframes;
    size_t num_keyframes;
};

struct material {
    struct texture *textures;
    size_t num_textures;
    struct effect *effects;
    size_t num_effects;
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
