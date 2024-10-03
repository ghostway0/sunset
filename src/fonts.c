#include <stdio.h>
#include <stdlib.h>

#include <log.h>

#include "sunset/fonts.h"

#define PSF2_MAGIC 0x864AB572

struct psf2_header {
    uint8_t magic[4];
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t length;
    uint32_t charsize;
    uint32_t height, width;
};

static int load_bitmaps(
        FILE *file, struct psf2_header const *header, struct font *font_out) {
    font_out->num_glyphs = header->length;
    font_out->glyphs = malloc(sizeof(struct glyph) * font_out->num_glyphs);

    uint8_t *bitmap = malloc(header->height * header->width);

    uint8_t row_size = (header->width + 7) / 8;

    for (size_t i = 0; i < font_out->num_glyphs; ++i) {
        font_out->glyphs[i].codepoint = i;
        font_out->glyphs[i].image.w = header->width;
        font_out->glyphs[i].image.h = header->height;
        font_out->glyphs[i].image.pixels =
                calloc(header->width * header->height, sizeof(struct color));

        fread(bitmap, header->height * header->width, 1, file);

        for (size_t y = 0; y < header->height; y++) {
            for (size_t x = 0; x < header->width; x++) {
                uint8_t byte = bitmap[y * row_size + x / 8];
                uint8_t bit = byte & (1 << (7 - (x % 8)));
                font_out->glyphs[i].image.pixels[y * header->width + x] =
                        bit ? COLOR_WHITE : COLOR_BLACK;
            }

            for (size_t i = header->height; i < row_size; i++) {
                if (bitmap[y * row_size + i] != 0) {
                    return -ERROR_PARSE;
                }
            }
        }

        fseek(file, header->charsize - header->height * row_size, SEEK_CUR);
    }

    free(bitmap);
    return 0;
}

int load_font_psf2(char const *path, char const *name, struct font *font_out) {
    int retval = 0;

    FILE *file = fopen(path, "rb");
    if (!file) {
        return -ERROR_OPEN_FILE;
    }

    struct psf2_header header;
    fread(&header, sizeof(header), 1, file);

    if (*(uint32_t *)header.magic != PSF2_MAGIC) {
        retval = -ERROR_PARSE;
        goto cleanup;
    }

    font_out->name = name;

    if ((retval = load_bitmaps(file, &header, font_out))) {
        goto cleanup;
    }

cleanup:
    fclose(file);
    return retval;
}

uint8_t convert_to_grayscale(struct color color) {
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

void show_image_grayscale(struct image const *image) {
    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel =
                    convert_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#%@"[pixel / 32]);
        }
        printf("\n");
    }
}

struct glyph const *font_get_glyph(
        struct font const *font, uint32_t codepoint) {
    if (font->num_glyphs < 256) {
        for (size_t i = 0; i < font->num_glyphs; i++) {
            if (font->glyphs[i].codepoint == codepoint) {
                return &font->glyphs[i];
            }
        }
    } else {
        size_t left = 0;
        size_t right = font->num_glyphs;

        while (left < right) {
            size_t mid = (left + right) / 2;
            if (font->glyphs[mid].codepoint == codepoint) {
                return &font->glyphs[mid];
            } else if (font->glyphs[mid].codepoint < codepoint) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        if (left < font->num_glyphs &&
            font->glyphs[left].codepoint == codepoint) {
            return &font->glyphs[left];
        }
    }

    return NULL;
}
