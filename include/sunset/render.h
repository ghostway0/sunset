#pragma once

#include "sunset/texture.h"
#include "sunset/vector.h"

enum uniform_type {
    UNIFORM_INT,
    UNIFORM_FLOAT,
    UNIFORM_VEC2,
    UNIFORM_VEC3,
    UNIFORM_VEC4,
    UNIFORM_MAT4,
};

struct render_context {
    vector(struct texture) textures;
    vector(struct uniform) uniforms;
};
