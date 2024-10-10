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

struct render_context {
    vector(struct texture) textures;
    vector(struct uniform) uniforms;
    vector(struct active_animation) active_animations;
};

int render(struct render_context *ctx,
        struct object *scene,
        struct camera *camera);
