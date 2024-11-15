#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "external/log.c/src/log.h"
#include "sunset/color.h"
#include "sunset/errors.h"
#include "sunset/geometry.h"
#include "sunset/tga.h"
#include "sunset/utils.h"

#define TGA_HEADER_SIZE 18
#define TGA_MAGIC "\x00\x00\x02\x00"

enum color_type {
    TGA_TYPE_INDEXED = 1,
    TGA_TYPE_RGB = 2,
    TGA_TYPE_GREY = 3,
    TGA_TYPE_RLE_INDEXED = 9,
    TGA_TYPE_RLE_RGB = 10,
    TGA_TYPE_RLE_GREY = 11,
};

struct tga_header {
    uint8_t id_length;
    uint8_t color_map_type;
    uint8_t image_type;
    uint16_t color_map_start;
    uint16_t color_map_length;
    uint8_t color_map_depth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t descriptor;
} __attribute__((packed));

int fill_color_map(uint8_t const *data,
        size_t pixel_size_bytes,
        size_t color_map_length,
        struct color *color_map_out) {
    assert(data != NULL);
    assert(color_map_out != NULL);

    for (size_t i = 0; i < color_map_length; i++) {
        uint8_t r = data[i * pixel_size_bytes + 2];
        uint8_t g = data[i * pixel_size_bytes + 1];
        uint8_t b = data[i * pixel_size_bytes + 0];
        uint8_t a =
                pixel_size_bytes == 4 ? data[i * pixel_size_bytes + 3] : 255;

        log_debug("%zu color_map: %d %d %d %d", i, r, g, b, a);

        color_map_out[i] = (struct color){r, g, b, a};
    }

    return 0;
}

struct tga_footer {
    uint32_t extension_offset;
    uint32_t developer_offset;
    char signature[18];
    char dot;
    char null;
} __attribute__((packed));

static int decompress_rle(uint8_t *data,
        size_t size,
        size_t pixel_size_bytes,
        struct color *out) {
    size_t offset = 0;
    struct color pixel;

    while (offset < size) {
        uint8_t packet_header = data[offset++];

        if (packet_header < 128) { // Raw pixel data
            packet_header++;
            for (int i = 0; i < packet_header; ++i) {
                memcpy(&pixel, data + offset, pixel_size_bytes);
                offset += pixel_size_bytes;
                *out++ = pixel;
            }
        } else { // RLE packet
            packet_header -= 127;
            memcpy(&pixel, data + offset, pixel_size_bytes);
            offset += pixel_size_bytes;
            for (int i = 0; i < packet_header; ++i) {
                *out++ = pixel;
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
        return -ERROR_INVALID_FORMAT;
    }

    struct tga_header *header = (struct tga_header *)(data + strlen(TGA_MAGIC));

    if (header->bpp % 8 != 0 || header->bpp >= 32) {
        return -ERROR_INVALID_FORMAT;
    }

    size_t image_size = header->width * header->height;
    size_t pixel_size_bytes = header->bpp / 8;

    log_debug("image_size: %zu %zu", image_size, header->bpp);

    image_out->pixels = sunset_calloc(image_size, sizeof(struct color));
    image_out->w = header->width;
    image_out->h = header->height;

    uint8_t *pixels = (uint8_t *)(data + TGA_HEADER_SIZE);

    ptrdiff_t footer_offset = strlen(TGA_MAGIC) + TGA_HEADER_SIZE
            + header->id_length + image_size * pixel_size_bytes
            + header->color_map_length * 2;

    if (data[2] == TGA_TYPE_RLE_RGB || data[2] == TGA_TYPE_RLE_GREY) {
        struct color *color_map = NULL;
        if (header->color_map_length > 0) {
            color_map = sunset_calloc(
                    header->color_map_length, sizeof(struct color));

            uint8_t const *color_map_data =
                    data + TGA_HEADER_SIZE + header->id_length;

            todo();

            for (size_t i = 0; i < header->color_map_length; i++) {
                color_map[i].r = color_map_data[i * pixel_size_bytes + 2];
                color_map[i].g = color_map_data[i * pixel_size_bytes + 1];
                color_map[i].b = color_map_data[i * pixel_size_bytes + 0];
                color_map[i].a = pixel_size_bytes == 4
                        ? color_map_data[i * pixel_size_bytes + 3]
                        : 255;
            }
        }

        if ((retval = decompress_rle(pixels,
                     image_size * pixel_size_bytes,
                     pixel_size_bytes,
                     image_out->pixels))
                != 0) {
            log_error("decompression");
            goto cleanup;
        }

        if (color_map) {
            free(color_map);
        }
    } else {
        for (size_t i = 0; i < image_size; i++) {
            if (pixel_size_bytes == 2) {
                image_out->pixels[i] =
                        color_from_16bit(*(uint16_t *)&pixels[2 * i]);
            } else {
                unreachable();
            }
        }
    }

    struct tga_footer *footer = (struct tga_footer *)(data + footer_offset);

    // if (footer->dot != '.') {
    //     log_info("%d", footer->dot);
    //     retval = -ERROR_INVALID_FORMAT;
    //     goto cleanup;
    // }

    log_debug("footer: %s", footer->signature);

cleanup:
    if (retval != 0) {
        free(image_out->pixels);
    }

    return retval;
}
