#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include "sunset/images.h"

#define vec3_format "vec3(%f, %f, %f)"
#define vec3_args(v) v[0], v[1], v[2]

#define rect_format "rect(%f, %f, %f, %f)"
#define rect_args(r) r.x, r.y, r.w, r.h

#define vec4_format "vec4(%f, %f, %f, %f)"
#define vec4_args(v) v[0], v[1], v[2], v[3]

#define mat4_format                                                        \
    "mat4(\n\t" vec4_format ",\n\t" vec4_format ",\n\t" vec4_format        \
    ",\n\t" vec4_format ",\n)"
#define mat4_fmt_args(m)                                                   \
    vec4_args(m[0]), vec4_args(m[1]), vec4_args(m[2]), vec4_args(m[3])

typedef struct Point {
    float x;
    float y;
} Point;

typedef struct Rect {
    float x;
    float y;
    float w;
    float h;
} Rect;

Rect rect_from_center(Point center, Point size);

Rect rect_closure(Rect a, Rect b);

bool is_zero_rect(Rect rect);

bool rect_contains(Rect parent, Rect child);

Point rect_center(Rect rect);

Point rect_size(Rect rect);

Point rect_get_origin(Rect rect);

// subdivide and get the ith quadrant
Rect rect_subdivide_i(Rect rect, size_t i, size_t n);

typedef struct AABB {
    vec3 min;
    vec3 max;
} AABB;

void aabb_translate(AABB *aabb, vec3 translation);

bool aabb_collide(AABB const *a, AABB const *b);

typedef struct Mesh {
    float *vertices;
    size_t num_vertices;
    uint32_t *indices;
    size_t num_indices;
    float *normals;
    size_t num_normals;
    float *texcoords;
    size_t num_texcoords;
} Mesh;

typedef struct Material {
    vec3 kd; // diffuse color
    vec3 ks; // specular color
    float ns; // specular exponent
    float d; // dissolve (transparency)
    Image map_kd; // diffuse texture map
    Image map_ke; // emission texture map
    char *name;
} Material;

AABB aabb_subdivide_i(AABB aabb, size_t i, size_t n);

bool aabb_contains_point(AABB aabb, vec3 point);

AABB from_rect(Rect rect);

bool point_within_rect(Point position, Rect rect);

bool position_within_aabb(vec3 position, AABB aabb);

float rect_distance_to_camera(vec3 camera_position, Rect rect);

float aabb_get_radius(AABB *aabb);

void aabb_get_center(AABB *aabb, vec3 center_out);

void aabb_extend_to(AABB *aabb, vec3 point);

#define aabb_format "aabb(min: " vec3_format ", max: " vec3_format ")"
#define aabb_args(b) vec3_args(b.min), vec3_args(b.max)

void aabb_closest_point(AABB const *aabb, vec3 point, vec3 closest_out);
