#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#define vec3_format "vec3(%f, %f, %f)"
#define vec3_args(v) v[0], v[1], v[2]

#define rect_format "rect(%zu, %zu, %zu, %zu)"
#define rect_args(r) r.x, r.y, r.width, r.height

#define vec4_format "vec4(%f, %f, %f, %f)"
#define vec4_args(v) v[0], v[1], v[2], v[3]

#define mat4_format                                                        \
    "mat4(\n\t" vec4_format ",\n\t" vec4_format ",\n\t" vec4_format        \
    ",\n\t" vec4_format ",\n)"
#define mat4_fmt_args(m)                                                   \
    vec4_args(m[0]), vec4_args(m[1]), vec4_args(m[2]), vec4_args(m[3])

struct point {
    float x;
    float y;
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

struct aabb {
    vec3 min;
    vec3 max;
};

void aabb_translate(struct aabb *aabb, vec3 translation);

bool aabb_collide(struct aabb const *a, struct aabb const *b);

struct mesh {
    float *vertices;
    size_t num_vertices;
    uint32_t *indices;
    size_t num_indices;
    float *normals;
    size_t num_normals;
    float *texcoords;
    size_t num_texcoords;
};

struct aabb aabb_subdivide_i(struct aabb aabb, size_t i, size_t n);

bool aabb_contains_point(struct aabb aabb, vec3 point);

struct aabb from_rect(struct rect rect);

bool point_within_rect(struct point position, struct rect rect);

bool position_within_aabb(vec3 position, struct aabb aabb);

float rect_distance_to_camera(vec3 camera_position, struct rect rect);

enum window_point {
    WINDOW_POINT_TOP_LEFT,
    WINDOW_POINT_TOP_RIGHT,
    WINDOW_POINT_BOTTOM_LEFT,
    WINDOW_POINT_BOTTOM_RIGHT,
};

float aabb_get_radius(struct aabb *aabb);

void aabb_get_center(struct aabb *aabb, vec3 center_out);

void aabb_extend_to(struct aabb *aabb, vec3 point);

#define aabb_format "aabb(min: " vec3_format ", max: " vec3_format ")"
#define aabb_args(b) vec3_args(b.min), vec3_args(b.max)

void aabb_closest_point(
        struct aabb const *aabb, vec3 point, vec3 closest_out);
