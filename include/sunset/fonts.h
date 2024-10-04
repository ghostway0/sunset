#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/gfx.h"

struct glyph {
    struct image image;
    struct rect bounds;
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

struct glyph const *font_get_glyph(struct font const *font, uint32_t codepoint);

void font_free(struct font *font);
