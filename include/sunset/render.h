#pragma once

#include <stddef.h>

#include "camera.h"
#include "scene.h"
#include "sunset/commands.h"
#include "vector.h"

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

struct context {
    struct command_buffer command_buffer;
    struct font *fonts;
    size_t num_fonts;
    void *render_context;
};
//
//
// int render(struct render_context *ctx,
//         struct object *scene,
//         struct camera *camera);
