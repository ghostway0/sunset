#pragma once

#include <stddef.h>

#include "sunset/geometry.h"

struct engine_context;

struct button {
    struct rect bounds;
    void (*clicked_callback)(struct engine_context *);
};

// engine context would have a vector of these and a pointer to the active one.
struct ui_context {
    struct button *buttons;
    size_t num_buttons;
};
