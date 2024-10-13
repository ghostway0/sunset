#pragma once

#include <stddef.h>

#include "camera.h"
#include "scene.h"
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

struct render_context {
    vector(struct texture) textures;
    vector(struct uniform) uniforms;
    vector(struct active_animation) active_animations;
    uint64_t handle;
    struct window window;
};

int render(struct render_context *ctx,
        struct object *scene,
        struct camera *camera);
