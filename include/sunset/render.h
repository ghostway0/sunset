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
    size_t width, height;
};

typedef void (*custom_command)(
        void *render_context, struct command const *command);

struct context {
    struct command_buffer command_buffer;
    struct font *fonts;
    size_t num_fonts;
    void *render_context;
    custom_command custom_commands[MAX_NUM_CUSTOM_COMMANDS];
};
//
//
// int render(struct render_context *ctx,
//         struct object *scene,
//         struct camera *camera);
