#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "sunset/color.h"
#include "sunset/geometry.h"
#include "sunset/tga.h"

#define TGA_HEADER_SIZE 18
#define TGA_MAGIC "\x00\x00\x02\x00"

enum tga_type {
    TGA_TYPE_INDEXED = 1,
    TGA_TYPE_RGB = 2,
    TGA_TYPE_GREY = 3,
    TGA_TYPE_RLE_INDEXED = 9,
    TGA_TYPE_RLE_RGB = 10,
    TGA_TYPE_RLE_GREY = 11,
};

static int decompress(uint8_t *data, size_t size, uint8_t *out, size_t pixel_size) {
    assert(data != NULL);
    assert(out != NULL);

    size_t i = 0;
    size_t j = 0;

    while (i < size) {
        uint8_t header = data[i++];
        uint8_t count = (header & 0x7F) + 1;

        if (header & 0x80) {
            // RLE packet: copy the same pixel 'count' times
            for (size_t k = 0; k < count; k++) {
                memcpy(out + j * pixel_size, data + i, pixel_size);
                j++;
            }
            i += pixel_size;
        } else {
            // Raw packet: copy 'count' pixels
            for (size_t k = 0; k < count; k++) {
                memcpy(out + j * pixel_size, data + i, pixel_size);
                j++;
                i += pixel_size;
            }
        }
    }

    return 0;
}

int load_tga_image(uint8_t const *data, struct image *image_out) {
    int retval = 0;

    assert(data != NULL);
    assert(image_out != NULL);

    if (memcmp(data, TGA_MAGIC, strlen(TGA_MAGIC)) != 0) {
        return -1;
    }

    uint16_t width = *(uint16_t *)(data + 12);
    uint16_t height = *(uint16_t *)(data + 14);
    uint8_t bpp = data[16];

    size_t pixel_size = bpp / 8;
    size_t image_size = width * height;

    image_out->w = width;
    image_out->h = height;
    image_out->pixels = calloc(image_size, sizeof(struct color));

    if (image_out->pixels == NULL) {
        retval = -1;
        goto cleanup;
    }

    uint8_t *pixels = (uint8_t *)(data + TGA_HEADER_SIZE);
    uint8_t *decompressed = malloc(image_size * pixel_size);

    if (decompressed == NULL) {
        retval = -1;
        goto cleanup;
    }

    if (data[2] == TGA_TYPE_RLE_RGB || data[2] == TGA_TYPE_RLE_GREY) {
        if ((retval = decompress(pixels, image_size * pixel_size, decompressed, pixel_size))) {
            free(decompressed);
            goto cleanup;
        }
    } else {
        memcpy(decompressed, pixels, image_size * pixel_size);
    }

    for (size_t i = 0; i < image_size; i++) {
        uint8_t r = decompressed[i * pixel_size + 2];
        uint8_t g = decompressed[i * pixel_size + 1];
        uint8_t b = decompressed[i * pixel_size + 0];
        uint8_t a = pixel_size == 4 ? decompressed[i * pixel_size + 3] : 255;

        image_out->pixels[i] = (struct color){r, g, b, a};
    }

    free(decompressed);

cleanup:
    if (retval != 0) {
        free(image_out->pixels);
    }

    return retval;
}
