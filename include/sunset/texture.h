#pragma once

#include <stddef.h>
#include <stdint.h>

struct texture {
    struct image *images;
    size_t num_images;
};
