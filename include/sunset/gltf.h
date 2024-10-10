#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cglm/cglm.h>
#include <cglm/types.h>

#include "sunset/geometry.h"
#include "vector.h"

#define GLB_MAGIC "0x46546C67"
#define GLB_VERSION 2
#define GLTF_VERSION "2.0"

#define parse_vecn_json(json, n, vec_out)                                      \
    do {                                                                       \
        json_assert_type(&json, JSON_ARRAY);                                   \
        if (vector_size(json.data.array) != n) {                               \
            return -ERROR_PARSE;                                               \
        }                                                                      \
        for (size_t i = 0; i < n; i++) {                                       \
            json_assert_type(&json.data.array[i], JSON_NUMBER);                \
            vec_out[i] = json.data.array[i].data.number;                       \
        }                                                                      \
    } while (0)

#define parse_matn_json(json, n, vec_out)                                      \
    do {                                                                       \
        json_assert_type(&json, JSON_ARRAY);                                   \
        if (vector_size(json.data.array) != n * n) {                           \
            return -ERROR_PARSE;                                               \
        }                                                                      \
        for (size_t i = 0; i < n; i++) {                                       \
            for (size_t j = 0; j < n; j++) {                                   \
                json_assert_type(&json.data.array[i * n + j], JSON_NUMBER);    \
                vec_out[i][j] = json.data.array[i * n + j].data.number;        \
            }                                                                  \
        }                                                                      \
    } while (0)

struct gltf_node {
    size_t camera;
    vector(size_t) children;
    size_t skin;
    mat4 matrix;
    size_t mesh;
    vec4 rotation;
    vec3 scale;
    vec3 translation;
    vector(size_t) weights;
    char const *name;
    // extensions
    // extras
};

struct gltf_scene {
    vector(size_t) nodes;
};

struct gltf_buffer {
    char *uri;
    size_t byte_length;
    char const *data;
};

struct gltf_buffer_view {
    size_t buffer;
    size_t byte_offset;
    size_t byte_length;
    size_t byte_stride;
    size_t target;
};

enum accessor_type {
    ACCESSOR_SCALAR,
    ACCESSOR_VEC2,
    ACCESSOR_VEC3,
    ACCESSOR_VEC4,
    ACCESSOR_MAT2,
    ACCESSOR_MAT3,
    ACCESSOR_MAT4,
};

struct accessor {
    size_t buffer_view;
    size_t byte_offset;
    size_t component_type;
    size_t count;
    enum accessor_type type;
    size_t *max;
    size_t *min;
};

enum primitive_attribute {
    PRIMITIVE_POSITION,
    PRIMITIVE_NORMAL,
    PRIMITIVE_TANGENT,
    PRIMITIVE_TEXCOORD_0,
    PRIMITIVE_TEXCOORD_1,
    PRIMITIVE_COLOR_0,
    PRIMITIVE_JOINTS_0,
    PRIMITIVE_WEIGHTS_0,
    PRIMITIVE_JOINTS_1,
    PRIMITIVE_WEIGHTS_1,
    PRIMITIVE_JOINTS_2,
    PRIMITIVE_WEIGHTS_2,
    PRIMITIVE_JOINTS_3,
    PRIMITIVE_WEIGHTS_3,
    NUM_PRIMITIVE_ATTRIBUTES,
};

struct primitive_attribute_kv {
    enum primitive_attribute key;
    size_t value;
};

enum primitive_type {
    PRIMITIVE_POINTS,
    PRIMITIVE_LINES,
    PRIMITIVE_LINE_LOOP,
    PRIMITIVE_LINE_STRIP,
    PRIMITIVE_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP,
    PRIMITIVE_TRIANGLE_FAN,
    NUM_PRIMITIVE_TYPES,
};

struct gltf_primitive {
    size_t material;
    size_t attributes[NUM_PRIMITIVE_ATTRIBUTES];
    size_t indices;
    enum primitive_type mode;
};

struct gltf_mesh {
    vector(struct gltf_primitive) primitives;
};

struct gltf_material {
    char *name;
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
};

enum animation_path_type {
    ANIMATION_PATH_TRANSLATION,
    ANIMATION_PATH_ROTATION,
    ANIMATION_PATH_SCALE,
    ANIMATION_PATH_WEIGHTS,
};

struct gltf_animation_target {
    size_t node;
    enum animation_path_type path;
};

struct gltf_animation_channel {
    size_t sampler;
    struct gltf_animation_target target;
};

struct gltf_animation_sampler {
    size_t input;
    size_t output;
    enum interpolation_type interpolation;
};

struct gltf_animation {
    char *name;
    vector(struct gltf_animation_channel) channels;
    vector(struct gltf_animation_sampler) samplers;
};

struct gltf_texture {
    size_t sampler;
    size_t source;
};

struct gltf_image {
    char *uri;
};

struct gltf_file {
    vector(struct gltf_scene) scenes;
    vector(struct gltf_node) nodes;
    vector(struct gltf_mesh) meshes;
    vector(struct gltf_buffer) buffers;
    vector(struct gltf_buffer_view) buffer_views;
    vector(struct accessor) accessors;
    vector(struct gltf_material) materials;
    vector(struct gltf_animation) animations;
    vector(struct gltf_image) images;
    vector(struct gltf_texture) textures;
};

int gltf_parse(FILE *file, struct gltf_file *file_out);

int gltf_load_file(char const *path, struct gltf_file *file_out);
