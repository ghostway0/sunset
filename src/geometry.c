#include <assert.h>
#include <stdint.h>

#include "cglm/types.h"
#include "cglm/vec3.h"
#include "sunset/geometry.h"
#include "sunset/math.h"

struct rect rect_from_center(struct point center, struct point size) {
    return (struct rect){
            .x = center.x - size.x / 2,
            .y = center.y - size.y / 2,
            .width = size.x,
            .height = size.y,
    };
}

struct point rect_center(struct rect rect) {
    return (struct point){
            .x = rect.x + rect.width / 2,
            .y = rect.y + rect.height / 2,
    };
}

struct point rect_size(struct rect rect) {
    return (struct point){rect.width, rect.height};
}

struct rect rect_subdivide_i(struct rect rect, size_t i, size_t n) {
    assert(i < n);
    assert(n > 0);

    size_t w = rect.width / n;
    size_t h = rect.height / n;

    return (struct rect){
            .x = rect.x + (i % n) * w,
            .y = rect.y + (i / n) * h,
            .width = w,
            .height = h,
    };
}

struct aabb aabb_subdivide_i(struct aabb aabb, size_t i, size_t n) {
    assert(i < n);
    assert(n > 0);

    struct aabb result = aabb;

    for (size_t j = 0; j < 3; j++) {
        float w = (aabb.max[j] - aabb.min[j]) / n;
        result.min[j] = aabb.min[j] + (i % n) * w;
        result.max[j] = aabb.min[j] + (i % n + 1) * w;
    }

    return result;
}

bool aabb_contains_point(struct aabb aabb, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        if (point[i] < aabb.min[i] || point[i] > aabb.max[i]) {
            return false;
        }
    }

    return true;
}

struct aabb from_rect(struct rect rect) {
    return (struct aabb){{rect.x, rect.y, 0.0f},
            {rect.x + rect.width, rect.y + rect.height, 0.0f}};
}

bool position_within_rect(vec3 position, struct rect rect) {
    return position[0] >= rect.x && position[0] <= rect.x + rect.width
            && position[1] >= rect.y && position[1] <= rect.y + rect.height;
}

bool position_within_aabb(vec3 position, struct aabb aabb) {
    return position[0] >= aabb.min[0] && position[0] <= aabb.max[0]
            && position[1] >= aabb.min[1] && position[1] <= aabb.max[1]
            && position[2] >= aabb.min[2] && position[2] <= aabb.max[2];
}

float rect_distance_to_camera(vec3 camera_position, struct rect rect) {
    vec3 center = {rect.x + (float)rect.width / 2,
            rect.y + (float)rect.height / 2,
            0.0f};

    return glm_vec3_distance(camera_position, center);
}

bool aabb_collide(struct aabb const *a, struct aabb const *b) {
    for (size_t i = 0; i < 3; i++) {
        if (a->max[i] < b->min[i] || a->min[i] > b->max[i]) {
            return false;
        }
    }

    return true;
}

void aabb_translate(struct aabb *aabb, vec3 translation) {
    glm_vec3_add(aabb->min, translation, aabb->min);
    glm_vec3_add(aabb->max, translation, aabb->max);
}

float aabb_get_radius(struct aabb *aabb) {
    vec3 height_2;
    glm_vec3_sub(aabb->max, aabb->min, height_2);
    glm_vec3_scale(height_2, sqrt(2) / 2, height_2);

    return glm_vec3_norm(height_2);
}

void aabb_get_center(struct aabb *aabb, vec3 center_out) {
    glm_vec3_add(aabb->min, aabb->max, center_out);
    glm_vec3_scale(center_out, 0.5, center_out);
}

void aabb_extend_to(struct aabb *aabb, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        aabb->min[i] = min(aabb->min[i], point[i]);
        aabb->max[i] = max(aabb->max[i], point[i]);
    }
}

void aabb_closest_point(struct aabb const *aabb, vec3 point, vec3 closest_out) {
    for (size_t i = 0; i < 3; i++) {
        closest_out[i] = fmax(aabb->min[i], fmin(point[i], aabb->max[i]));
    }
}
