#pragma once

#include <stddef.h>
#include <stdint.h>

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

// struct mouse {
//     struct point where;
//     bool first_mouse;
// };
//
// struct user_context {
//     struct mouse mouse;
//     struct event_queue *event_queue;
//     struct scene *scene;
// };
//
// // should this structure be split into user_context and engine_context?
// (also
// // maybe a struct ui) user_context would be given to callbacks and such.
// struct context {
//     // the backend should handle these
//     struct font *fonts;
//     size_t num_fonts;
//
//     void *render_context;
//
//     vector(struct ui_context) ui_contexts;
//     struct ui_context *active_ui;
//
//     struct physics physics;
//
//     struct user_context user_context;
// };
