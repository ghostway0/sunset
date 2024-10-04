#include <stdint.h>
#include <stdio.h>

#include "sunset/gfx.h"
#include "sunset/color.h"

void show_image_grayscale(struct image const *image) {
    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel =
                    color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }
}

void show_image_grayscale_at(struct image const *image, struct point pos) {
    for (size_t y = 0; y < image->h; y++) {
        printf("\033[%lu;%luH", pos.y + y, pos.x);

        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel =
                    color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }

    printf("\033[%lu;%luH", pos.y + image->h, (size_t)1);
}
