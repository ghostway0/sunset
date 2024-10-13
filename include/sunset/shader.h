#pragma once

#include <stddef.h>
#include <stdio.h>

#include <cglm/vec3.h>

#include "byte_stream.h"

enum argument_type {
    ARGUMENT_UNIFORM_INT,
    ARGUMENT_UNIFORM_FLOAT,
    ARGUMENT_UNIFORM_VEC3,
    ARGUMENT_UNIFORM_MAT4,
    ARGUMENT_UNIFORM_BUFFEr,
    ARGUMENT_DYNAMIC,
};

struct shader_argument {
    enum argument_type type;
    char const *name;
    size_t size;
};

struct active_argument {
    char const *name;
    struct byte_stream data;
};

struct program {
    struct shader_argument *arguments;
    size_t num_arguments;
    uint64_t handle;
};

struct shader_signature {
    struct uniform *uniforms;
    size_t num_uniforms;
    struct ssbo *ssbos;
    size_t num_ssbos;
};
