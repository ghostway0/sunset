#pragma once

#include <stddef.h>
#include <stdio.h>

#include <cglm/vec3.h>

#include "byte_stream.h"

enum uniform_type {
    UNIFORM_I32 = 0,
    UNIFORM_F32,
    UNIFORM_F64,
    UNIFORM_BYTE,

    // keep this last
    NUM_UNIFORM_TYPES,
};

extern size_t const uniform_sizes[NUM_UNIFORM_TYPES];

struct uniform {
    char const *name;
    enum uniform_type lane_type;
    size_t lanes;
};

struct shader {
    struct uniform *uniforms;
    size_t num_uniforms;
    void *handle;
};

void shader_print_signature(struct shader const *shader, FILE *stream);

size_t shader_arguments_size(struct shader const *shader);

int backend_launch_shader(void *handle, struct byte_stream *arguments);

int shader_launch(struct shader *shader, struct byte_stream *arguments);
