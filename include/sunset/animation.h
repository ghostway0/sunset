#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <cglm/types.h>

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
