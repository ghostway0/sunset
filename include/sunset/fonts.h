#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/images.h"

struct glyph {
    Image image;
    struct rect bounds;
    int advance_x;
};

typedef struct Font {
    struct glyph *glyphs;
    uint32_t *glyph_map;
    size_t num_glyphs;
} Font;

int load_font_psf2(char const *path, Font *font_out);

struct glyph const *font_get_glyph(Font const *font, uint32_t codepoint);

void font_destroy(Font *font);
