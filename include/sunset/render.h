#pragma once

#include <stddef.h>

#include "sunset/commands.h"
#include "sunset/geometry.h"
#include "sunset/physics.h"
#include "sunset/ui.h"

typedef struct EventQueue EventQueue;
struct button;
struct scene;
struct font;

struct active_animation {
    float start_time;
    struct animation *animation;
    size_t curr_keyframe;
};

struct window {
    size_t width;
    size_t height;

    uint64_t handle;
};

struct render_config {
    size_t window_width, window_height;
    char const *window_title;
};

struct mouse {
    struct point where;
    bool first_mouse;
};

// should this structure be split into user_context and engine_context? (also
// maybe a struct ui) user_context would be given to callbacks and such.
struct context {
    struct command_buffer command_buffer;
    // the backend should handle these
    struct font *fonts;
    size_t num_fonts;

    void *render_context;
    struct scene *scene;

    Vector(struct ui_context) ui_contexts;
    struct ui_context *active_ui;

    struct mouse mouse;
    EventQueue *event_queue;
};
