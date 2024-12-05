#pragma once

#include <stddef.h>

#include "sunset/commands.h"

struct scene;
struct font;
struct event_queue;

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
    float x;
    float y;
    bool first_mouse;
};

struct context {
    struct command_buffer command_buffer;
    struct font *fonts;
    size_t num_fonts;
    void *render_context;
    struct scene *scene;

    struct mouse mouse;
    struct event_queue *event_queue;
};
