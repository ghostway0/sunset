#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cglm/types.h"
#include "cglm/vec3.h"
#include "sunset/color.h"
#include "sunset/geometry.h"
#include "sunset/math.h"

void show_image_grayscale(struct image const *image) {
    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel = color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }
}

void show_image_grayscale_at(struct image const *image, struct point pos) {
    for (size_t y = 0; y < image->h; y++) {
        printf("\033[%u;%uH", pos.y + (uint32_t)y, pos.x);

        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel = color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }

    printf("\033[%lu;%luH", pos.y + image->h, (size_t)1);
}

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

struct box box_subdivide_i(struct box box, size_t i, size_t n) {
    assert(i < n);
    assert(n > 0);

    struct box result = box;

    for (size_t j = 0; j < 3; j++) {
        float w = (box.max[j] - box.min[j]) / n;
        result.min[j] = box.min[j] + (i % n) * w;
        result.max[j] = box.min[j] + (i % n + 1) * w;
    }

    return result;
}

bool box_contains_point(struct box box, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        if (point[i] < box.min[i] || point[i] > box.max[i]) {
            return false;
        }
    }

    return true;
}

struct box from_rect(struct rect rect) {
    return (struct box){{rect.x, rect.y, 0.0f},
            {rect.x + rect.width, rect.y + rect.height, 0.0f}};
}

bool position_within_rect(vec3 position, struct rect rect) {
    return position[0] >= rect.x && position[0] <= rect.x + rect.width
            && position[1] >= rect.y && position[1] <= rect.y + rect.height;
}

bool position_within_box(vec3 position, struct box box) {
    return position[0] >= box.min[0] && position[0] <= box.max[0]
            && position[1] >= box.min[1] && position[1] <= box.max[1]
            && position[2] >= box.min[2] && position[2] <= box.max[2];
}

float rect_distance_to_camera(vec3 camera_position, struct rect rect) {
    vec3 center = {rect.x + (float)rect.width / 2,
            rect.y + (float)rect.height / 2,
            0.0f};

    return glm_vec3_distance(camera_position, center);
}

bool box_collide(struct box const *a, struct box const *b) {
    for (size_t i = 0; i < 3; i++) {
        if (a->max[i] < b->min[i] || a->min[i] > b->max[i]) {
            return false;
        }
    }

    return true;
}

void box_translate(struct box *box, vec3 translation) {
    glm_vec3_add(box->min, translation, box->min);
    glm_vec3_add(box->max, translation, box->max);
}

float box_get_radius(struct box *box) {
    vec3 height_2;
    glm_vec3_sub(box->max, box->min, height_2);
    glm_vec3_scale(height_2, sqrt(2) / 2, height_2);

    return glm_vec3_norm(height_2);
}

void box_get_center(struct box *box, vec3 center_out) {
    glm_vec3_add(box->min, box->max, center_out);
    glm_vec3_scale(center_out, 0.5, center_out);
}

void box_extend_to(struct box *box, vec3 point) {
    for (size_t i = 0; i < 3; i++) {
        box->min[i] = min(box->min[i], point[i]);
        box->max[i] = max(box->max[i], point[i]);
    }
}

void box_closest_point(struct box const *box, vec3 point, vec3 closest_out) {
    for (size_t i = 0; i < 3; i++) {
        closest_out[i] = fmax(box->min[i], fmin(point[i], box->max[i]));
    }
}
