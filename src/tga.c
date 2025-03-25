#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <log.h>

#include "sunset/errors.h"
#include "sunset/images.h"
#include "sunset/io.h"
#include "sunset/vector.h"

#include "sunset/tga.h"

enum color_type {
    TGA_TYPE_INDEXED = 1,
    TGA_TYPE_RGB = 2,
    TGA_TYPE_GREY = 3,
    TGA_TYPE_RLE_INDEXED = 9,
    TGA_TYPE_RLE_RGB = 10,
    TGA_TYPE_RLE_GREY = 11,
};

typedef struct TGAHeader {
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
} __attribute__((packed)) TGAHeader;

int fill_color_map(uint8_t const *data,
        size_t pixel_size_bytes,
        size_t color_map_length,
        Color *color_map_out) {
    assert(data != NULL);
    assert(color_map_out != NULL);

    for (size_t i = 0; i < color_map_length; i++) {
        uint8_t r = data[i * pixel_size_bytes + 2];
        uint8_t g = data[i * pixel_size_bytes + 1];
        uint8_t b = data[i * pixel_size_bytes + 0];
        uint8_t a = pixel_size_bytes == 4 ? data[i * pixel_size_bytes + 3]
                                          : 255;

        color_map_out[i] = (Color){r, g, b, a};
    }

    return 0;
}

typedef struct TGAFooter {
    uint32_t extension_offset;
    uint32_t developer_offset;
    char signature[18];
    char dot;
    char null;
} __attribute__((packed)) TGAFooter;

static int decompress_rle(Reader *reader,
        size_t decompressed_size,
        size_t pixel_size_bytes,
        Color *out) {
    size_t decompressed_read = 0;
    uint8_t pixel[pixel_size_bytes];

    while (decompressed_read < decompressed_size) {
        uint8_t packet_header;
        reader_readall(reader, 1, &packet_header);

        if (packet_header < 128) { // Raw pixel data
            packet_header++;
            for (size_t i = 0; i < packet_header; i++) {
                decompressed_read +=
                        reader_readall(reader, pixel_size_bytes, pixel);
                *out++ = color_from_bytes(pixel, pixel_size_bytes);
            }
        } else { // RLE packet
            packet_header -= 127;
            decompressed_read +=
                    reader_readall(reader, pixel_size_bytes, pixel)
                    * packet_header;
            Color color = color_from_bytes(pixel, pixel_size_bytes);
            for (size_t i = 0; i < packet_header; i++) {
                *out++ = color;
            }
        }
    }

    return 0;
}

int tga_load_image(Reader *reader, Image *image_out) {
    int retval = 0;

    TGAHeader header;
    if (reader_read(reader, sizeof(TGAHeader), &header)
            != sizeof(TGAHeader)) {
        return -ERROR_IO;
    }

    if (header.bpp % 8 != 0 || header.bpp > 32) {
        return -ERROR_INVALID_FORMAT;
    }

    size_t image_size = header.width * header.height;
    size_t pixel_size_bytes = header.bpp / 8;

    image_out->pixels = sunset_calloc(image_size, sizeof(Color));
    image_out->w = header.width;
    image_out->h = header.height;
    reader_skip(reader, header.id_length);

    if (header.image_type == TGA_TYPE_RLE_RGB
            || header.image_type == TGA_TYPE_RLE_GREY) {
        Color *color_map = NULL;
        if (header.color_map_length > 0) {
            color_map =
                    sunset_calloc(header.color_map_length, sizeof(Color));

            // Read color map data
            vector(uint8_t) color_map_data;
            vector_init(color_map_data);
            reader_read_to_vec(reader,
                    header.color_map_length * pixel_size_bytes,
                    &color_map_data);

            // Fill color map (using fill_color_map function)
            if ((retval = fill_color_map(color_map_data,
                         pixel_size_bytes,
                         header.color_map_length,
                         color_map))
                    != 0) {
                vector_destroy(color_map_data);
                goto cleanup;
            }
            vector_destroy(color_map_data);
        }

        if ((retval = decompress_rle(reader,
                     image_size * pixel_size_bytes,
                     pixel_size_bytes,
                     image_out->pixels))
                != 0) {
            goto cleanup;
        }

        if (color_map) {
            free(color_map);
        }
    } else {
        // Handle uncompressed data (currently only supports 16-bit)
        for (size_t i = 0; i < image_size; i++) {
            uint16_t pixel_data;
            if (reader_read(reader, sizeof(uint16_t), &pixel_data)
                    != sizeof(uint16_t)) {
                retval = ERROR_IO;
                break;
            }

            if (pixel_size_bytes == 2) {
                image_out->pixels[i] = color_from_16bit(pixel_data);
            } else {
                unreachable();
            }
        }
    }

cleanup:
    if (retval != 0) {
        free(image_out->pixels);
    }

    return retval;
}
