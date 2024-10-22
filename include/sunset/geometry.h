#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#define vec3_format "vec3(%f, %f, %f)"
#define vec3_args(v) v[0], v[1], v[2]

#define rect_format "rect(%zu, %zu, %zu, %zu)"
#define rect_args(r) r.x, r.y, r.width, r.height

struct point {
    size_t x;
    size_t y;
};

struct rect {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
};

struct rect rect_from_center(struct point center, struct point size);

struct point rect_center(struct rect rect);

struct point rect_size(struct rect rect);

// subdivide and get the ith quadrant
struct rect rect_subdivide_i(struct rect rect, size_t i, size_t n);

bool position_within_rect(vec3 position, struct rect rect);

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
    struct image image;
    vec2 *coords;
    size_t num_coords;
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

struct box box_subdivide_i(struct box box, size_t i, size_t n);

bool box_contains_point(struct box box, vec3 point);

struct box from_rect(struct rect rect);

bool position_within_rect(vec3 position, struct rect rect);

bool position_within_box(vec3 position, struct box box);

float rect_distance_to_camera(vec3 camera_position, struct rect rect);
