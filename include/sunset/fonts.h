#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/color.h"
#include "sunset/gfx.h"

struct glyph {
    struct image image;
    int x0;
    int y0;
    int x1;
    int y1;
    int advance_x;
    int advance_y;
};

struct font {
    char const *name;
    struct glyph *glyphs;
    uint16_t *glyph_map;
    size_t num_glyphs;
};

int load_font_psf2(char const *path, char const *name, struct font *font_out);

void show_image_grayscale(struct image const *image);

void show_image_grayscale_at(struct image const *image, struct point pos);

struct glyph const *font_get_glyph(struct font const *font, uint32_t codepoint);

void font_free(struct font *font);
