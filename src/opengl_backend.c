#include "opengl_backend.h"
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

int backend_setup(struct render_context *context, struct render_config config) {
    if (!glfwInit()) {
        return -ERROR_IO;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    context->window =
            glfwCreateWindow(config.width, config.height, "Sunset", NULL, NULL);

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

int backend_create_program(struct program *program_out) {
    GLuint program = glCreateProgram();
    if (!program) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    program_out->handle = program;
    return 0;
}

int backend_program_add_shader(struct program *program,
        char const *source,
        enum shader_type shader_type,
        struct shader_argument const *arguments,
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

int compile_mesh(struct mesh const *mesh, struct compiled_mesh *mesh_out) {
    glGenVertexArrays(1, &mesh_out->vao);
    glGenBuffers(1, &mesh_out->vbo);
    glGenBuffers(1, &mesh_out->ebo);

    glBindVertexArray(mesh_out->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh_out->vbo);
    glBufferData(GL_ARRAY_BUFFER,
            mesh->num_vertices * sizeof(vec3),
            mesh->vertices,
            GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_out->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            mesh->num_indices * sizeof(uint32_t),
            mesh->indices,
            GL_STATIC_DRAW);

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    mesh_out->num_indices = mesh->num_indices;

    return 0;
}

void backend_draw_mesh(struct compiled_mesh *mesh) {
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);
}

void backend_draw(struct render_context *context) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(context->window);
    glfwPollEvents();
}

void backend_free(struct render_context *context) {
    glfwDestroyWindow(context->window);
    glfwTerminate();
}

void backend_free_program(struct program *program) {
    glDeleteProgram((GLuint)program->handle);
}

void compiled_mesh_destroy(struct compiled_mesh *mesh) {
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}

int backend_compile_texture(struct texture const *texture,
        struct compiled_texture *compiled_texture) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_RGB,
            texture->image.w,
            texture->image.h,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            texture->image.pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    compiled_texture->tex = tex;

    return 0;
}

char const *point_light_fragment_shader =
        "#version 450 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 light_position;\n"
        "uniform vec3 light_color;\n"
        "uniform float light_intensity;\n"
        "uniform vec3 object_color;\n"
        "void main() {\n"
        "}\n";

const struct shader_argument point_light_arguments[] = {
        {.name = "light_position", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "object_color", .type = ARGUMENT_UNIFORM_VEC3},
};

char const *directional_light_fragment_shader =
        "#version 450 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 light_direction;\n"
        "uniform vec3 light_color;\n"
        "uniform float light_intensity;\n"
        "uniform vec3 object_color;\n"
        "void main() {\n"
        "}\n";

const struct shader_argument directional_light_arguments[] = {
        {.name = "light_direction", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "object_color", .type = ARGUMENT_UNIFORM_VEC3},
};

char const *ambient_light_fragment_shader =
        "#version 450 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 light_color;\n"
        "uniform float light_intensity;\n"
        "uniform vec3 object_color;\n"
        "void main() {\n"
        "}\n";

const struct shader_argument ambient_light_arguments[] = {
        {.name = "light_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "object_color", .type = ARGUMENT_UNIFORM_VEC3},
};

char const *spotlight_light_fragment_shader = 
    "#version 450 core"

    "out vec4 FragColor;"

    "uniform vec3 light_position;"
    "uniform vec3 light_direction;"
    "uniform float light_angle;"
    "uniform vec3 light_color;"
    "uniform float light_intensity;"
    "uniform vec3 object_color;"

    "in vec3 Normal;"
    "in vec3 FragPos;"

    "void main() {"
    "}";

const struct shader_argument spotlight_light_arguments[] = {
        {.name = "light_position", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_direction", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_angle", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "light_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "object_color", .type = ARGUMENT_UNIFORM_VEC3},
};

// this is pretty horrible code
int backend_compile_light_shader(
        struct light const *light, struct program *program) {
    int retval = 0;

    char const *vertex_shader =
            "#version 450 core\n"
            "layout (location = 0) in vec3 position;\n"
            "uniform mat4 mvp;\n"
            "void main() {\n"
            "    gl_Position = mvp * vec4(position, 1.0);\n"
            "}\n";

    struct shader_argument vertex_arguments[] = {
            {.name = "mvp", .type = ARGUMENT_UNIFORM_MAT4},
    };

    if (backend_create_program(program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

    if (compile_shader_into(vertex, vertex_shader)) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    char const *fragment_source = NULL;
    struct shader_argument const *fragment_arguments = NULL;

    switch (light->type) {
        case LIGHT_POINT:
            fragment_source = point_light_fragment_shader;
            fragment_arguments = point_light_arguments;
            break;
        case LIGHT_DIRECTIONAL:
            fragment_source = directional_light_fragment_shader;
            fragment_arguments = directional_light_arguments;
            break;
        case LIGHT_AMBIENT:
            fragment_source = ambient_light_fragment_shader;
            fragment_arguments = ambient_light_arguments;
            break;
        case LIGHT_SPOTLIGHT:
            fragment_source = spotlight_light_fragment_shader;
            fragment_arguments = spotlight_light_arguments;
            break;
        default:
            unreachable();
    }

    if (compile_shader_into(fragment, fragment_source)) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    if (backend_program_add_shader(program,
                vertex_shader,
                GL_VERTEX_SHADER,
                vertex_arguments,
                sizeof(vertex_arguments) / sizeof(vertex_arguments[0]))) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    if (backend_program_add_shader(program,
                fragment_source,
                GL_FRAGMENT_SHADER,
                fragment_arguments,
                sizeof(point_light_arguments)
                        / sizeof(point_light_arguments[0]))) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    if (backend_link_program(program)) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    return 0;

failure:
    glDeleteProgram(program->handle);
    return retval;
}
