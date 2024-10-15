#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "sunset/color.h"
#include "sunset/geometry.h"

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
        printf("\033[%lu;%luH", pos.y + y, pos.x);

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
