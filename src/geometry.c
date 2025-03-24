#include <assert.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include "internal/math.h"
#include "internal/mem_utils.h"
#include "internal/utils.h"

#include "sunset/geometry.h"

Rect rect_from_center(Point center, Point size) {
    return (Rect){
            .x = center.x - size.x / 2,
            .y = center.y - size.y / 2,
            .w = size.x,
            .h = size.y,
    };
}

bool is_zero_rect(Rect rect) {
    return rect.x == 0.0 && rect.y == 0.0 && rect.h == 0.0 && rect.w == 0.0;
}

Rect rect_closure(Rect a, Rect b) {
    float min_x = min(a.x, b.x);
    float min_y = min(a.y, b.y);

    float max_x = max(a.x + a.w, b.x + b.w);
    float max_y = max(a.y + a.h, b.y + b.h);

    return (Rect){
            .x = min_x,
            .y = min_y,
            .h = max_y - min_y,
            .w = max_x - min_x,
    };
}

bool rect_contains(Rect parent, Rect child) {
    return child.x >= parent.x && child.y >= parent.y
            && (child.x + child.w) <= (parent.x + parent.w)
            && (child.y + child.h) <= (parent.y + parent.h);
}

Point rect_center(Rect rect) {
    return (Point){
            .x = rect.x + rect.w / 2,
            .y = rect.y + rect.h / 2,
    };
}

Point rect_size(Rect rect) {
    return (Point){rect.w, rect.h};
}

Point rect_get_origin(Rect rect) {
    return (Point){.x = rect.x, .y = rect.y};
}

Rect rect_subdivide_i(Rect rect, size_t i, size_t n) {
    assert(i < n);
    assert(n > 0);

    size_t w = rect.w / n;
    size_t h = rect.h / n;

    return (Rect){
            .x = rect.x + (i % n) * w,
            .y = rect.y + (i / n) * h,
            .w = w,
            .h = h,
    };
}

AABB aabb_subdivide_i(AABB aabb, size_t i, size_t n) {
    assert(i < n);
    assert(n > 0);

    AABB result = aabb;

    for (size_t j = 0; j < 3; j++) {
        float w = (aabb.max[j] - aabb.min[j]) / n;
        result.min[j] = aabb.min[j] + (i % n) * w;
        result.max[j] = aabb.min[j] + (i % n + 1) * w;
    }

    return result;
}

bool aabb_contains_point(AABB aabb, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        if (point[i] < aabb.min[i] || point[i] > aabb.max[i]) {
            return false;
        }
    }

    return true;
}

AABB from_rect(Rect rect) {
    return (AABB){{rect.x, rect.y, 0.0f},
            {rect.x + rect.w, rect.y + rect.h, 0.0f}};
}

bool point_within_rect(Point position, Rect rect) {
    return position.x >= rect.x && position.x <= rect.x + rect.w
            && position.y >= rect.y && position.y <= rect.y + rect.h;
}

bool position_within_aabb(vec3 position, AABB aabb) {
    return position[0] >= aabb.min[0] && position[0] <= aabb.max[0]
            && position[1] >= aabb.min[1] && position[1] <= aabb.max[1]
            && position[2] >= aabb.min[2] && position[2] <= aabb.max[2];
}

float rect_distance_to_camera(vec3 camera_position, Rect rect) {
    vec3 center = {
            rect.x + (float)rect.w / 2, rect.y + (float)rect.h / 2, 0.0f};

    return glm_vec3_distance(camera_position, center);
}

bool aabb_collide(AABB const *a, AABB const *b) {
    for (size_t i = 0; i < 3; i++) {
        if (a->max[i] < b->min[i] || a->min[i] > b->max[i]) {
            return false;
        }
    }

    return true;
}

void aabb_translate(AABB *aabb, vec3 translation) {
    glm_vec3_add(aabb->min, translation, aabb->min);
    glm_vec3_add(aabb->max, translation, aabb->max);
}

float aabb_get_radius(AABB *aabb) {
    vec3 height_2;
    glm_vec3_sub(aabb->max, aabb->min, height_2);
    glm_vec3_scale(height_2, sqrt(2) / 2, height_2);

    return glm_vec3_norm(height_2);
}

void aabb_get_center(AABB *aabb, vec3 center_out) {
    glm_vec3_add(aabb->min, aabb->max, center_out);
    glm_vec3_scale(center_out, 0.5, center_out);
}

void aabb_extend_to(AABB *aabb, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        aabb->min[i] = min(aabb->min[i], point[i]);
        aabb->max[i] = max(aabb->max[i], point[i]);
    }
}

void aabb_closest_point(AABB const *aabb, vec3 point, vec3 closest_out) {
    for (size_t i = 0; i < 3; i++) {
        closest_out[i] = fmax(aabb->min[i], fmin(point[i], aabb->max[i]));
    }
}

bool ray_intersects_aabb(vec3 ray_origin,
        vec3 const ray_dir,
        const AABB *box,
        vec3 hit_out) {
    float tmin = 0.0f;
    float tmax = FLT_MAX;

    for (int i = 0; i < 3; i++) {
        float inv_dir = 1.0f / (ray_dir[i] + EPSILON);

        float t1 = (box->min[i] - ray_origin[i]) * inv_dir;
        float t2 = (box->max[i] - ray_origin[i]) * inv_dir;
        if (t1 > t2) {
            swap(t1, t2);
        }

        tmin = max(tmin, t1);
        tmax = min(tmax, t2);
    }

    if (hit_out) {
        glm_vec3_copy(ray_origin, hit_out);
        glm_vec3_muladds(ray_origin, tmin, hit_out);
    }

    return tmax >= tmin;
}

void aabb_get_face_center(
        const AABB *aabb, const vec3 axis, vec3 face_center_out) {
    vec3 center;
    aabb_get_center((AABB *)aabb, center);
    glm_vec3_copy(center, face_center_out);

    if (fabsf(axis[0]) > 0.0f) {
        face_center_out[0] = (axis[0] > 0.0f) ? aabb->max[0] : aabb->min[0];
    }
    if (fabsf(axis[1]) > 0.0f) {
        face_center_out[1] = (axis[1] > 0.0f) ? aabb->max[1] : aabb->min[1];
    }
    if (fabsf(axis[2]) > 0.0f) {
        face_center_out[2] = (axis[2] > 0.0f) ? aabb->max[2] : aabb->min[2];
    }
}

void aabb_scale(AABB *aabb, float factor) {
    glm_vec3_scale(aabb->min, factor, aabb->min);
    glm_vec3_scale(aabb->max, factor, aabb->max);
}
