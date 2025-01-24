#pragma once

typedef struct Reader Reader;
typedef struct Image Image;

int tga_load_image(Reader *reader, Image *image_out);
