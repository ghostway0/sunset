#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "sunset/color.h"
#include "sunset/gfx.h"

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
            .pos =
                    {
                            .x = center.x - size.x / 2,
                            .y = center.y - size.y / 2,
                    },
            .w = size.x,
            .h = size.y,
    };
}

struct point rect_center(struct rect rect) {
    return (struct point){
            .x = rect.pos.x + rect.w / 2,
            .y = rect.pos.y + rect.h / 2,
    };
}

struct point rect_size(struct rect rect) {
    return (struct point){rect.w, rect.h};
}

struct rect rect_subdivide_i(struct rect rect, size_t i) {
    assert(i < 4);

    struct point center = rect_center(rect);
    struct point size = rect_size(rect);

    struct point new_center = {
            .x = center.x + (i & 1 ? size.x : -size.x) / 4,
            .y = center.y + (i & 2 ? size.y : -size.y) / 4,
    };

    return rect_from_center(new_center, (struct point){size.x / 2, size.y / 2});
}
