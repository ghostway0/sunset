#pragma once

struct reader;
struct image;

int tga_load_image(struct reader *reader, struct image *image_out);
