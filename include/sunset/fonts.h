#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/images.h"

struct glyph {
    struct image image;
    struct rect bounds;
    int advance_x;
};

struct font {
    struct glyph *glyphs;
    uint32_t *glyph_map;
    size_t num_glyphs;
};

int load_font_psf2(char const *path, struct font *font_out);

struct glyph const *font_get_glyph(
        struct font const *font, uint32_t codepoint);

void font_destroy(struct font *font);
