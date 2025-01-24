#pragma once

#include <stddef.h>
#include <stdint.h>

struct texture {
    Image *images;
    size_t num_images;
};
