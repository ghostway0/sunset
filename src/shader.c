#include <stddef.h>

#include "sunset/errors.h"
#include "sunset/shader.h"

size_t const uniform_sizes[NUM_UNIFORM_TYPES] = {
        [UNIFORM_INT] = 4,
        [UNIFORM_FLOAT] = 4,
        [UNIFORM_VEC2] = 8,
        [UNIFORM_VEC3] = 12,
        [UNIFORM_VEC4] = 16,
        [UNIFORM_MAT4] = 64,
};

size_t shader_arguments_size(struct shader const *shader) {
    size_t sum = 0;

    for (size_t i = 0; i < shader->num_uniforms; i++) {
        struct uniform const *uniform = &shader->uniforms[i];

        if (uniform->type == UNIFORM_ARBT) {
            sum += uniform->size;
            continue;
        }

        sum += uniform_sizes[uniform->type];
    }

    return sum;
}

int shader_launch(struct shader *shader, struct byte_stream *arguments) {
    size_t size = shader_arguments_size(shader);

    if (arguments->size != size) {
        return -ERROR_INVALID_ARGUMENTS;
    }

    // return backend_launch_shader(shader->handle, arguments);
    return 1;
}

static char const *uniform_type_names[NUM_UNIFORM_TYPES] = {
        [UNIFORM_INT] = "int",
        [UNIFORM_FLOAT] = "float",
        [UNIFORM_VEC2] = "vec2",
        [UNIFORM_VEC3] = "vec3",
        [UNIFORM_VEC4] = "vec4",
        [UNIFORM_MAT4] = "mat4",
        [UNIFORM_ARBT] = "arbt",
};

void shader_print_signature(struct shader const *shader, FILE *stream) {
    for (size_t i = 0; i < shader->num_uniforms; i++) {
        struct uniform const *uniform = &shader->uniforms[i];

        if (uniform->type == UNIFORM_ARBT) {
            fprintf(stream, "arbt(%zu) %s", uniform->size, uniform->name);
        } else {
            fprintf(stream,
                    "%s %s",
                    uniform_type_names[uniform->type],
                    uniform->name);
        }

        if (i < shader->num_uniforms - 1) {
            fprintf(stream, ", ");
        }
    }
}
