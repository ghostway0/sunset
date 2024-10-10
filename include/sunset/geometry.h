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

enum interpolation_type {
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBICSPLINE,
    INTERPOLATION_STEP,
};

enum keyframe_type {
    KEYFRAME_TRANSFORM,
    KEYFRAME_MORPH,
    KEYFRAME_WEIGHTS,
    KEYFRAME_VISIBILITY,
    KEYFRAME_COLOR,
    KEYFRAME_INTENSITY,
    KEYFRAME_TEXTURE,
    KEYFRAME_MATERIAL,
};

struct keyframe_transform {
    vec3 position;
    vec3 rotation;
    float scale;
    enum interpolation_type position_interpolation;
    enum interpolation_type rotation_interpolation;
    enum interpolation_type scale_interpolation;
};

struct keyframe_morph {
    size_t target;
    float weight;
    enum interpolation_type weight_interpolation;
};

struct keyframe_weights {
    size_t target;
    float weight;
    enum interpolation_type weight_interpolation;
};

struct keyframe_visibility {
    bool visible;
    enum interpolation_type interpolation;
};

struct keyframe_color {
    vec4 color;
    enum interpolation_type interpolation;
};

struct keyframe_intensity {
    float intensity;
    enum interpolation_type interpolation;
};

struct keyframe_texture {
    size_t target;
    struct texture *texture;
    enum interpolation_type interpolation;
};

struct keyframe_material {
    size_t target;
    struct material *material;
    enum interpolation_type interpolation;
};

struct keyframe {
    enum keyframe_type type;
    union {
        struct keyframe_transform transform;
        struct keyframe_morph morph;
        struct keyframe_weights weights;
        struct keyframe_visibility visibility;
        struct keyframe_color color;
        struct keyframe_intensity intensity;
        struct keyframe_texture texture;
        struct keyframe_material material;
    } data;
};

struct animation {
    char const *name;
    float duration;
    struct keyframe *keyframes;
    size_t num_keyframes;
};

struct active_animation {
    float start_time;
    struct animation *animation;
    size_t curr_keyframe;
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
