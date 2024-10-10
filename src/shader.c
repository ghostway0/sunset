#include <stddef.h>

#include "sunset/errors.h"
#include "sunset/shader.h"

size_t const uniform_sizes[NUM_UNIFORM_TYPES] = {
        [UNIFORM_I32] = sizeof(int),
        [UNIFORM_F32] = sizeof(float),
        [UNIFORM_F64] = sizeof(double),
        [UNIFORM_BYTE] = sizeof(char),
};

size_t shader_arguments_size(struct shader const *shader) {
    size_t sum = 0;

    for (size_t i = 0; i < shader->num_uniforms; i++) {
        struct uniform const *uniform = &shader->uniforms[i];

        sum += uniform_sizes[uniform->lane_type] * uniform->lanes;
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
        [UNIFORM_I32] = "i32",
        [UNIFORM_F32] = "f32",
        [UNIFORM_F64] = "f64",
        [UNIFORM_BYTE] = "byte",
};

void shader_print_signature(struct shader const *shader, FILE *stream) {
    for (size_t i = 0; i < shader->num_uniforms; i++) {
        struct uniform const *uniform = &shader->uniforms[i];

        if (uniform->lanes > 1) {
            fprintf(stream,
                    "%s: %s[%zu]",
                    uniform->name,
                    uniform_type_names[uniform->lane_type],
                    uniform->lanes);
        } else {
            fprintf(stream,
                    "%s: %s",
                    uniform->name,
                    uniform_type_names[uniform->lane_type]);
        }

        if (i < shader->num_uniforms - 1) {
            fprintf(stream, ", ");
        }
    }
}
