#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/crc32.h"
#include "sunset/errors.h"
#include "sunset/images.h"
#include "sunset/io.h"
#include "sunset/vector.h"

#define PNG_IHDR 0x52444849 // "IHDR"
#define PNG_PLTE 0x45544C50 // "PLTE"
#define PNG_IDAT 0x54414449 // "IDAT"
#define PNG_IEND 0x444E4549 // "IEND"
#define PNG_tRNS 0x74524E53 // "tRNS"
#define PNG_MAGIC 0x0a1a0a0d0474e5089

typedef struct PNGHeader {
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
} __attribute__((packed)) PNGHeader;

typedef struct PNGChunkHeader {
    uint32_t length;
    uint32_t type;
} __attribute__((packed)) PNGChunkHeader;

typedef struct PNGData {
    PNGHeader header;
    vector(Color) palette;
    vector(uint8_t) idat_data;
} PNGData;

static int parse_ihdr(Reader *reader, PNGHeader *header_out) {
    int err;

    if ((err = reader_read(reader, sizeof(*header_out), header_out))) {
        return err;
    }

    if (header_out->bit_depth != 8) {
        return -ERROR_UNSUPPORTED;
    }

    if (header_out->compression_method != 0
            || header_out->filter_method != 0
            || header_out->interlace_method != 0) {
        return -ERROR_UNSUPPORTED;
    }

    return 0;
}

static int parse_plte(Reader *reader,
        PNGChunkHeader *chunk_header,
        vector(Color) * palette_out) {
    int err = 0;

    if (chunk_header->length % 3 != 0) {
        return -ERROR_INVALID_FORMAT;
    }

    size_t num_colors = chunk_header->length / 3;
    vector_resize(*palette_out, num_colors);

    for (size_t i = 0; i < num_colors; ++i) {
        struct {
            uint8_t r, g, b;
        } color;

        if ((err = reader_read(reader, sizeof(color), &color))) {
            return err;
        }

        (*palette_out)[i] = (Color){color.r, color.g, color.b, 255};
    }

    return 0;
}

static int parse_idat(Reader *reader,
        PNGChunkHeader *chunk_header,
        vector(uint8_t) * data_out) {
    size_t old_size = vector_size(*data_out);
    vector_resize(*data_out, old_size + chunk_header->length);

    if (reader_read(reader, chunk_header->length, *data_out + old_size)
            != chunk_header->length) {
        return -ERROR_IO;
    }

    return 0;
}

static int parse_trns(Reader *reader,
        PNGChunkHeader *chunk_header,
        PNGHeader const *png_header,
        vector(Color) * palette_out) {
    int err = 0;

    // stream reader through gzip deflator?

    if (png_header->color_type == 0) {
        if (chunk_header->length != 2) {
            return -ERROR_INVALID_FORMAT;
        }
        uint16_t transparent_gray;
        if ((err = reader_read(
                     reader, sizeof(uint16_t), &transparent_gray))) {
            return err;
        }
        // TODO: Implement handling for grayscale transparency
    } else if (png_header->color_type == 2) {
        if (chunk_header->length != 6) {
            return -ERROR_INVALID_FORMAT;
        }
        struct {
            uint16_t r, g, b;
        } transparent_color;
        if ((err = reader_read(reader,
                     sizeof(transparent_color),
                     &transparent_color))) {
            return err;
        }
        // TODO: Implement handling for truecolor transparency
    } else if (png_header->color_type == 3) {
        if (chunk_header->length > vector_size(palette_out)) {
            return -ERROR_INVALID_FORMAT;
        }
        for (size_t i = 0; i < chunk_header->length; ++i) {
            uint8_t alpha;
            if ((err = reader_read(reader, sizeof(uint8_t), &alpha))) {
                return err;
            }
            if (i < vector_size(palette_out)) {
                (*palette_out)[i].a = alpha;
            }
        }
    } else if (png_header->color_type == 4 || png_header->color_type == 6) {
        // tRNS is not used with color types that already have alpha
        return -ERROR_INVALID_FORMAT;
    }

    return 0;
}

int png_decode(Reader *reader, PNGData *png_data_out) {
    int retval = 0;

    vector_init(png_data_out->palette);
    vector_init(png_data_out->idat_data);
    memset(&png_data_out->header, 0, sizeof(png_data_out->header));

    while (true) {
        PNGChunkHeader chunk_header;
        if (reader_read(reader, sizeof(chunk_header), &chunk_header)
                != sizeof(chunk_header)) {
            retval = -ERROR_INVALID_FORMAT;
            goto cleanup;
        }

        chunk_header.length = __builtin_bswap32(chunk_header.length);
        chunk_header.type = __builtin_bswap32(chunk_header.type);

        vector(uint8_t) chunk_data;
        vector_init(chunk_data);

        vector_resize(chunk_data, chunk_header.length);

        if (reader_read(reader, chunk_header.length, chunk_data)
                != chunk_header.length) {
            vector_destroy(chunk_data);
            retval = -ERROR_IO;
            goto cleanup;
        }

        uint32_t expected_crc;
        if (reader_read(reader, sizeof(uint32_t), &expected_crc)) {
            vector_destroy(chunk_data);
            retval = -ERROR_IO;
            goto cleanup;
        }

        expected_crc = __builtin_bswap32(expected_crc);

        uint32_t actual_crc = crc32((uint8_t const *)&chunk_header.type,
                sizeof(chunk_header.type));
        actual_crc = crc32_from_seed(
                actual_crc, chunk_data, vector_size(chunk_data));

        if (actual_crc != expected_crc) {
            vector_destroy(chunk_data);
            retval = -ERROR_INVALID_FORMAT;
            goto cleanup;
        }

        if (chunk_header.type == PNG_IHDR) {
            if ((retval = parse_ihdr(reader, &png_data_out->header)) != 0) {
                vector_destroy(chunk_data);
                goto cleanup;
            }
        } else if (chunk_header.type == PNG_PLTE) {
            if ((retval = parse_plte(
                         reader, &chunk_header, &png_data_out->palette))
                    != 0) {
                vector_destroy(chunk_data);
                goto cleanup;
            }
        } else if (chunk_header.type == PNG_IDAT) {
            if ((retval = parse_idat(
                         reader, &chunk_header, &png_data_out->idat_data))
                    != 0) {
                vector_destroy(chunk_data);
                goto cleanup;
            }
        } else if (chunk_header.type == PNG_tRNS) {
            if ((retval = parse_trns(reader,
                         &chunk_header,
                         &png_data_out->header,
                         &png_data_out->palette))
                    != 0) {
                vector_destroy(chunk_data);
                goto cleanup;
            }
        } else if (chunk_header.type == PNG_IEND) {
            vector_destroy(chunk_data);
            break;
        }

        vector_destroy(chunk_data);
    }

cleanup:
    if (retval != 0) {
        vector_destroy(png_data_out->palette);
        vector_destroy(png_data_out->idat_data);
    }

    return retval;
}

int png_process_pixels(PNGData const *png_data, struct image *image_out) {
    assert(png_data != NULL);
    assert(image_out != NULL);

    image_out->w = png_data->header.width;
    image_out->h = png_data->header.height;
    image_out->pixels =
            sunset_calloc(image_out->w * image_out->h, sizeof(Color));

    int retval = 0;
    PNGHeader const *header = &png_data->header;
    vector(uint8_t) idat_data = png_data->idat_data;
    vector(Color) palette = png_data->palette;

    if (header->color_type == 0) { // Grayscale
        // TODO: Implement grayscale processing
        retval = -ERROR_UNSUPPORTED;
    } else if (header->color_type == 2) { // Truecolor
        if (vector_size(idat_data) != header->width * header->height * 3) {
            retval = -ERROR_INVALID_FORMAT;
        } else {
            for (size_t i = 0; i < header->width * header->height; ++i) {
                image_out->pixels[i].r = (idat_data)[i * 3 + 0];
                image_out->pixels[i].g = (idat_data)[i * 3 + 1];
                image_out->pixels[i].b = (idat_data)[i * 3 + 2];
                image_out->pixels[i].a = 255;
            }
        }
    } else if (header->color_type == 3) { // Indexed color
        if (vector_size(palette) == 0) {
            retval = -ERROR_INVALID_FORMAT;
        } else if (vector_size(idat_data)
                != header->width * header->height) {
            retval = -ERROR_INVALID_FORMAT;
        } else {
            for (size_t i = 0; i < header->width * header->height; ++i) {
                if ((idat_data)[i] >= vector_size(palette)) {
                    retval = -ERROR_INVALID_FORMAT;
                    break;
                }
                image_out->pixels[i] = (palette)[(idat_data)[i]];
            }
        }
    } else if (header->color_type == 4) { // Grayscale with alpha
        // TODO: Implement grayscale with alpha processing
        retval = -ERROR_UNSUPPORTED;
    } else if (header->color_type == 6) { // Truecolor with alpha
        if (vector_size(idat_data) != header->width * header->height * 4) {
            retval = -ERROR_INVALID_FORMAT;
        } else {
            for (size_t i = 0; i < header->width * header->height; ++i) {
                image_out->pixels[i] = (Color){
                        .r = (idat_data)[i * 4 + 0],
                        .g = (idat_data)[i * 4 + 1],
                        .b = (idat_data)[i * 4 + 2],
                        .a = (idat_data)[i * 4 + 3],
                };
            }
        }
    } else {
        retval = -ERROR_UNSUPPORTED;
    }

    if (retval != 0 && image_out->pixels != NULL) {
        free(image_out->pixels);
        image_out->pixels = NULL;
    }

    return retval;
}

int png_load_image(Reader *reader, struct image *image_out) {
    int retval;

    uint64_t signature;
    if (reader_read(reader, sizeof(signature), &signature)
            != sizeof(signature)) {
        return -ERROR_IO;
    }

    if (signature != PNG_MAGIC) {
        return -ERROR_INVALID_FORMAT;
    }

    PNGData png_data;
    if ((retval = png_decode(reader, &png_data))) {
        goto cleanup;
    }

    if ((retval = png_process_pixels(&png_data, image_out))) {
        goto cleanup;
    }

cleanup:
    vector_destroy(png_data.palette);
    vector_destroy(png_data.idat_data);

    return retval;
}
