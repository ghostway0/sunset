#include <stdint.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "sunset/backend.h"
#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/utils.h"

static int compile_shader_into(GLuint shader, char const *source) {
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    return 0;
}
//
// static int set_uniform_arguments(struct shader *shader,
//         struct uniform_argument *arguments,
//         size_t num_arguments) {
//     for (size_t i = 0; i < num_arguments; i++) {
//         struct uniform_argument *uniform = &arguments[i];
//
//         if (uniform->type == UNIFORM_UBO) {
//             GLuint buffer;
//
//             glGenBuffers(1, &buffer);
//             glBindBuffer(GL_UNIFORM_BUFFER, buffer);
//             glBufferData(GL_UNIFORM_BUFFER,
//                     uniform->data.size,
//                     uniform->data.data,
//                     GL_STATIC_DRAW);
//
//             GLuint block_index = glGetUniformBlockIndex(
//                     (GLuint)shader->handle, uniform->name);
//             if (block_index == GL_INVALID_INDEX) {
//                 return -ERROR_UNIFORM_NOT_FOUND;
//             }
//
//             glUniformBlockBinding((GLuint)shader->handle, block_index, i);
//
//             GLint location = glGetUniformBlockIndex(
//                     (GLuint)shader->handle, uniform->name);
//
//             glBindBufferRange(GL_UNIFORM_BUFFER,
//                     location,
//                     buffer,
//                     0,
//                     uniform->data.size);
//
//             continue;
//         }
//
//         GLint location =
//                 glGetUniformLocation((GLuint)shader->handle, uniform->name);
//
//         if (location == -1) {
//             return -ERROR_UNIFORM_NOT_FOUND;
//         }
//
//         switch (uniform->type) {
//             case UNIFORM_INT: {
//                 int value;
//                 byte_stream_read(&uniform->data, mat4, &value);
//                 glUniform1i(location, value);
//                 break;
//             }
//             case UNIFORM_FLOAT: {
//                 float value;
//                 byte_stream_read(&uniform->data, mat4, &value);
//                 glUniform1f(location, value);
//                 break;
//             }
//             case UNIFORM_MAT4: {
//                 mat4 value;
//                 byte_stream_read(&uniform->data, mat4, &value);
//                 glUniformMatrix4fv(location, 1, GL_FALSE, (float *)value);
//                 break;
//             }
//             default:
//                 return -ERROR_INVALID_ARGUMENTS;
//         }
//     }
//
//     return 0;
// }
//
// static int set_ssbo_arguments(struct shader *shader,
//         struct ssbo_argument *arguments,
//         size_t num_arguments) {
//     GLuint buffer;
//
//     for (size_t i = 0; i < num_arguments; i++) {
//         struct ssbo_argument *ssbo = &arguments[i];
//
//         GLuint block_index = glGetProgramResourceIndex(
//                 (GLuint)shader->handle, GL_SHADER_STORAGE_BLOCK, ssbo->name);
//
//         if (block_index == GL_INVALID_INDEX) {
//             return -ERROR_UNIFORM_NOT_FOUND;
//         }
//
//         glGenBuffers(1, &buffer);
//         glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
//         glBufferData(GL_SHADER_STORAGE_BUFFER,
//                 ssbo->data->size,
//                 ssbo->data->data,
//                 GL_STATIC_DRAW);
//
//         glShaderStorageBlockBinding((GLuint)shader->handle, block_index, i);
//         glBindBufferRange(
//                 GL_SHADER_STORAGE_BUFFER, i, buffer, 0, ssbo->data->size);
//     }
//
//     return 0;
// }
//
int backend_setup(struct render_context *context, struct render_config config) {
    if (!glfwInit()) {
        return -ERROR_IO;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    context->window = glfwCreateWindow(
            config.width, config.height, "Sunset", NULL, NULL);

    glfwMakeContextCurrent(context->window);

    if (!context->window) {
        return -ERROR_IO;
    }

    if (glewInit() != GLEW_OK) {
        return -ERROR_IO;
    }

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    return 0;
}
//
// int backend_create_shader(char const *source,
//         enum shader_type shader_type,
//         struct shader_signature signature,
//         struct shader *shader_out) {
//     GLuint program = glCreateProgram();
//     int err = 0;
//
//     GLuint shader = glCreateShader(shader_type);
//
//     if ((err = compile_shader_into(shader, source))) {
//         goto cleanup;
//     }
//
//     glAttachShader(program, shader);
//     glLinkProgram(program);
//
//     GLint success;
//     glGetProgramiv(program, GL_LINK_STATUS, &success);
//
//     if (!success) {
//         err = -ERROR_SHADER_COMPILATION_FAILED;
//         goto cleanup;
//     }
//
//     shader_out->handle = program;
//
//     shader_out->uniforms = signature.uniforms;
//     shader_out->num_uniforms = signature.num_uniforms;
//
//     shader_out->ssbos = signature.ssbos;
//     shader_out->num_ssbos = signature.num_ssbos;
//
// cleanup:
//     if (err != 0) {
//         glDeleteShader(shader);
//         glDeleteProgram(program);
//     }
//
//     return err;
// }
//
// int backend_setup_shader(
//         struct shader *shader, struct shader_arguments *arguments) {
//     int err = 0;
//
//     if ((err = set_uniform_arguments(
//                  shader, arguments->uniforms, arguments->num_uniforms))) {
//         return err;
//     }
//
//     if ((err = set_ssbo_arguments(
//                  shader, arguments->ssbos, arguments->num_ssbos))) {
//         return err;
//     }
//
//     // glUseProgram(shader->handle);
//
//     return 0;
// }

int backend_create_program(struct program *program_out) {
    GLuint program = glCreateProgram();
    if (!program) {
        return -ERROR_OUT_OF_MEMORY;
    }

    program_out->handle = program;
    return 0;
}

int backend_program_add_shader(struct program *program,
        char const *source,
        enum shader_type shader_type,
        struct shader_argument *arguments,
        size_t num_arguments) {
    unused(arguments);
    unused(num_arguments);

    GLuint shader = glCreateShader(shader_type);
    if (!shader) {
        return -ERROR_OUT_OF_MEMORY;
    }

    if (compile_shader_into(shader, source)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glAttachShader((GLuint)program->handle, shader);

    return 0;
}

int backend_link_program(struct program *program) {
    glLinkProgram((GLuint)program->handle);

    GLint success;
    glGetProgramiv((GLuint)program->handle, GL_LINK_STATUS, &success);

    if (!success) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    return 0;
}

int backend_set_program_arguments(struct program *program,
        struct active_argument *arguments,
        size_t num_arguments) {
    for (size_t i = 0; i < num_arguments; i++) {
        struct active_argument *argument = &arguments[i];

        GLint location =
                glGetUniformLocation((GLuint)program->handle, argument->name);

        if (location == -1) {
            return -ERROR_UNIFORM_NOT_FOUND;
        }

        switch (argument->type) {
            case ARGUMENT_UNIFORM_INT: {
                int value;
                byte_stream_read(&argument->data, int, &value);
                glUniform1i(location, value);
                break;
            }
            case ARGUMENT_UNIFORM_FLOAT: {
                float value;
                byte_stream_read(&argument->data, float, &value);
                glUniform1f(location, value);
                break;
            }
            case ARGUMENT_UNIFORM_VEC3: {
                vec3 value;
                byte_stream_read(&argument->data, vec3, &value);
                glUniform3fv(location, 1, (float *)value);
                break;
            }
            case ARGUMENT_UNIFORM_MAT4: {
                mat4 value;
                byte_stream_read(&argument->data, mat4, &value);
                glUniformMatrix4fv(location, 1, GL_FALSE, (float *)value);
                break;
            }
            case ARGUMENT_UNIFORM_BUFFER: {
                GLuint buffer;
                glGenBuffers(1, &buffer);
                glBindBuffer(GL_UNIFORM_BUFFER, buffer);
                glBufferData(GL_UNIFORM_BUFFER,
                        argument->data.size,
                        argument->data.data,
                        GL_STATIC_DRAW);

                glUniformBlockBinding((GLuint)program->handle, location, i);

                glBindBufferRange(GL_UNIFORM_BUFFER,
                        location,
                        buffer,
                        0,
                        argument->data.size);
                break;
            }
            case ARGUMENT_DYNAMIC: {
                break;
            }
            default:
                return -ERROR_INVALID_ARGUMENTS;
        }
    }

    return 0;
}

int backend_setup_shader(struct program *program,
        struct active_argument *arguments,
        size_t num_arguments) {
    if (backend_set_program_arguments(program, arguments, num_arguments)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    return 0;
}
