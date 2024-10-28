#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <log.h>

#include "sunset/color.h"
#include "sunset/errors.h"
#include "sunset/geometry.h"

#include "sunset/fonts.h"

#define PSF2_MAGIC 0x864AB572

enum psf2_flags {
    PSF2_HAS_UNICODE_TABLE = 1 << 0,
    PSF2_HAS_DEFAULT_WIDTH = 1 << 1,
    PSF2_HAS_DEFAULT_HEIGHT = 1 << 2,
    PSF2_HAS_WIDTH = 1 << 3,
    PSF2_HAS_HEIGHT = 1 << 4,
    PSF2_HAS_MAX_WIDTH = 1 << 5,
    PSF2_HAS_MAX_HEIGHT = 1 << 6,
    PSF2_HAS_WIDTH_HEIGHT = 1 << 7,
    PSF2_HAS_FONT_SIZE = 1 << 8,
    PSF2_HAS_XY_OFFSET = 1 << 9,
    PSF2_HAS_UNICODE = 1 << 10,
    PSF2_HAS_FONT_BBX = 1 << 11,
};

struct psf2_header {
    uint8_t magic[4];
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t length;
    uint32_t charsize;
    uint32_t height, width;
};

static void flip_image(struct image *image) {
    struct color *flipped_pixels =
            malloc(image->w * image->h * sizeof(struct color));

    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            flipped_pixels[(image->h - y - 1) * image->w + x] =
                    image->pixels[y * image->w + x];
        }
    }

    free(image->pixels);
    image->pixels = flipped_pixels;
}

static int load_glyphs(
        FILE *file, struct psf2_header const *header, struct font *font_out) {
    uint8_t *bitmap = malloc(header->height * header->width);
    uint8_t row_size = (header->width + 7) / 8;

    for (size_t i = 0; i < font_out->num_glyphs; ++i) {
        font_out->glyph_map[i] = i;
        font_out->glyphs[i].image.w = header->width;
        font_out->glyphs[i].image.h = header->height;
        font_out->glyphs[i].image.pixels =
                calloc(header->width * header->height, sizeof(struct color));

        font_out->glyphs[i].bounds = (struct rect){
                .x = 0,
                .y = 0,
                .width = header->width,
                .height = header->height,
        };

        font_out->glyphs[i].advance_x = header->width;

        fread(bitmap, header->height * row_size, 1, file);

        for (size_t y = 0; y < header->height; y++) {
            for (size_t x = 0; x < header->width; x++) {
                uint8_t byte = bitmap[y * row_size + x / 8];
                uint8_t bit = byte & (1 << (7 - (x % 8)));
                font_out->glyphs[i].image.pixels[y * header->width + x] =
                        bit ? COLOR_WHITE : COLOR_BLACK;
            }
        }

        flip_image(&font_out->glyphs[i].image);

        fseek(file, header->charsize - header->height * row_size, SEEK_CUR);
    }

    free(bitmap);
    return 0;
}

static int correct_glyph_table(FILE *file, struct font *font_out) {
    for (size_t glyph_index = 0; glyph_index < font_out->num_glyphs;
            glyph_index++) {
        uint8_t length;
        fread(&length, 1, 1, file);

        if (feof(file)) {
            break;
        }

        if (length == 0xFF) {
            continue;
        }

        for (uint8_t i = 0; i < length; ++i) {
            uint32_t unicode_code_point;
            if (fread(&unicode_code_point, sizeof(uint32_t), 1, file) != 1) {
                break;
            }

            if (unicode_code_point > 0x10FFFF) {
                // should not happen, but does?
                continue;
            }

            font_out->glyph_map[unicode_code_point] = glyph_index;
        }
    }

    return 0;
}

int load_font_psf2(char const *path, char const *name, struct font *font_out) {
    int retval = 0;

    FILE *file = fopen(path, "rb");
    if (!file) {
        return -ERROR_IO;
    }

    struct psf2_header header;
    fread(&header, sizeof(header), 1, file);

    if (*(uint32_t *)header.magic != PSF2_MAGIC) {
        retval = -ERROR_INVALID_FORMAT;
        goto cleanup;
    }

    font_out->name = name;
    font_out->num_glyphs = header.length;
    font_out->glyphs = malloc(sizeof(struct glyph) * font_out->num_glyphs);
    font_out->glyph_map = calloc(0x10FFFF, sizeof(uint32_t));

    if ((retval = load_glyphs(file, &header, font_out))) {
        goto cleanup;
    }

    fseek(file,
            font_out->num_glyphs * header.charsize + header.headersize,
            SEEK_SET);

    if (header.flags & PSF2_HAS_UNICODE_TABLE) {
        retval = correct_glyph_table(file, font_out);
    }

cleanup:
    fclose(file);
    return retval;
}

struct glyph const *font_get_glyph(
        struct font const *font, uint32_t codepoint) {
    if (codepoint > 0x10FFFF) {
        return NULL;
    }

    size_t glyph_index = font->glyph_map[codepoint];

    if (glyph_index >= font->num_glyphs) {
        return NULL;
    }

    return &font->glyphs[glyph_index];
}

void font_free(struct font *font) {
    for (size_t i = 0; i < font->num_glyphs; ++i) {
        free(font->glyphs[i].image.pixels);
    }

    free(font->glyphs);
    free(font->glyph_map);
}
