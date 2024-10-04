#pragma once

#include <stddef.h>

struct point {
    size_t x;
    size_t y;
};

struct rect {
    struct point pos;
    int w;
    int h;
};
