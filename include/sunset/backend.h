#pragma once

#include <stddef.h>
#include <stdint.h>

#include "byte_stream.h"
#include "shader.h"
#include "render.h"

#include "config.h"

enum argument_type {
    ARGUMENT_UNIFORM,
    ARGUMENT_SSBO,
};

struct ssbo_argument {
    char const *name;
    struct byte_stream *data;
};

struct shader_arguments {
    struct uniform_argument *uniforms;
    size_t num_uniforms;
    struct ssbo_argument *ssbos;
    size_t num_ssbos;
};

int backend_setup_shader(
        struct shader *shader, struct shader_arguments *arguments);

int backend_create_shader(char const *source,
        enum shader_type shader_type,
        struct shader_signature signature,
        struct shader *shader_out);

int backend_setup(
        struct render_context *context, struct render_config config);
