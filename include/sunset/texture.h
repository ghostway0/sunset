#pragma once

#include <stdint.h>
#include <stddef.h>

struct texture {
    struct image *images;
    size_t num_images;
};
