#pragma once

typedef struct Reader Reader;
struct image;

int tga_load_image(Reader *reader, struct image *image_out);
