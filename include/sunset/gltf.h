#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cglm/cglm.h>

#include "gfx.h"
#include "vector.h"

#define GLB_MAGIC "0x46546C67"
#define GLB_VERSION 2
#define GLTF_VERSION "2.0"

struct node {
    size_t mesh;
    size_t *children;
    size_t num_children;
    vec3 translation;
    vec4 rotation[4];
    vec3 scale[3];
};

struct gltf_scene {
    struct node *nodes;
    size_t num_nodes;
};

struct gltf_buffer {
    char const *uri;
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

struct material {
    char const *name;
};

struct gltf_file {
    vector(struct gltf_scene) scenes;
    vector(struct node) nodes;
    vector(struct gltf_mesh) meshes;
    vector(struct gltf_buffer) buffers;
    vector(struct gltf_buffer_view) buffer_views;
    vector(struct accessor) accessors;
};

int gltf_parse(FILE *file, struct gltf_file *file_out);
