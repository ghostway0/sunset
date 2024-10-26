#include "opengl_backend.h"
#include <stdint.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <time.h>

#include "log.h"
#include "sunset/backend.h"
#include "sunset/byte_stream.h"
#include "sunset/config.h"
#include "sunset/errors.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

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

static int setup_default_shaders(struct program *program_out) {
    char const *vertex =
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPos, 1.0);\n"
            "}\n";

    char const *fragment =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
            "}\n";

    backend_create_program(program_out);

    backend_program_add_shader(program_out, vertex, SHADER_VERTEX);
    backend_program_add_shader(program_out, fragment, SHADER_FRAGMENT);

    backend_link_program(program_out);

    return 0;
}

void backend_use_program(struct program const *program) {
    glUseProgram(program->handle);
}

int backend_setup(struct render_context *context, struct render_config config) {
    if (!glfwInit()) {
        return -ERROR_IO;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    context->window =
            glfwCreateWindow(config.width, config.height, "Sunset", NULL, NULL);

    if (!context->window) {
        return -ERROR_IO;
    }

    glfwMakeContextCurrent(context->window);

    if (glewInit() != GLEW_OK) {
        return -ERROR_IO;
    }

    glEnable(GL_DEPTH_TEST);

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    struct program default_program;
    setup_default_shaders(&default_program);
    backend_use_program(&default_program);

    vector_init(context->instanced_meshes, struct instanced_mesh);

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
        enum shader_type shader_type) {
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
    glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}
/*
    vector_resize(context->instanced_meshes,
            vector_size(context->instanced_meshes) + 1);
    struct instanced_mesh *instanced_mesh =
            &context->instanced_meshes[vector_size(context->instanced_meshes)
                    - 1];

    instanced_mesh->mesh = *compiled_mesh;
    vector_init(instanced_mesh->transforms, mat4);

    glGenBuffers(1, &instanced_mesh->transform_buffer);


        glBufferData(GL_ARRAY_BUFFER,
                vector_size(instanced_mesh->transforms) * sizeof(mat4),
                instanced_mesh->transforms,
                GL_STATIC_DRAW);

    return instanced_mesh;
 * */

void backend_setup_instanced_mesh(
        struct instanced_mesh *mesh, mat4 *transforms, size_t num_transforms) {
    glBindBuffer(GL_ARRAY_BUFFER, mesh->transform_buffer);
    glBufferData(GL_ARRAY_BUFFER,
            num_transforms * sizeof(mat4),
            transforms,
            GL_STATIC_DRAW);

    glBindVertexArray(mesh->mesh.vao);
    glDrawElementsInstanced(GL_TRIANGLES,
            mesh->mesh.num_indices,
            GL_UNSIGNED_INT,
            NULL,
            num_transforms);

    glBindVertexArray(0);
}

/*
 * Render Pipeline Plan:
 *  - Create a render context (backend_setup): create a window and set up OpenGL
 *
 *  To create shaders, I'll have a struct program which is a collection of shaders.
 *  `backend_create_program` and `backend_program_add_shader` will be used to create
 *  a program and add shaders to it. `backend_link_program` will link the program.
 *
 *  To set shader arguments, I'll have a struct active_argument which is a collection
 *  of arguments. `backend_set_program_arguments` will be used to set the arguments.
 *
 *  I think shaders should have standard arguments, like mvp, and then custom arguments
 *  can be set by the user.
 *
 *
 *  How can I prompt a shader once? like for a particle system (although particles may span
 *  multiple frames. how does that work?)
 *
 *  Seperation:
 *    struct object is the scene node. should I every frame go over the active objects and put out commands?
 *    That sounds like the most separated structure I could have. Then a frame builder would hold instanced 
 *    stuff for example. we should have the frame cache within render context (like instanced meshes ids).
 *
 *    meshes in objects should be in ids: they should be registered with the backend and that returns an id
 *    this would enable easy instancing.
 * */

void backend_draw(struct render_context *context, mat4 view, mat4 projection) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: this is a bit of a hack

    for (size_t i = 0; i < vector_size(context->meshes); i++) {
        backend_draw_mesh(&context->meshes[i]);
    }

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

static GLuint compile_texture(struct texture const *texture) {
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

    // if (texture->wrap_s == TEXTURE_REPEAT) {
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // } else {
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // }

    return tex;
}

char const *point_light_fragment_shader = "";

const struct shader_argument point_light_arguments[] = {};

char const *directional_light_vertex_shader = "";

char const *directional_light_fragment_shader = "";

const struct shader_argument directional_light_arguments[] = {};

char const *ambient_light_fragment_shader =
        "#version 450 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 light_color;\n"
        "uniform float light_intensity;\n"
        "uniform vec3 object_color;\n"
        "uniform float ambient_intensity;\n"
        "void main() {\n"
        "    FragColor = vec4(light_color * light_intensity * object_color * "
        "ambient_intensity, 1.0);\n"
        "}\n";

const struct shader_argument ambient_light_arguments[] = {
        {.name = "light_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "light_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
        {.name = "object_color", .type = ARGUMENT_UNIFORM_VEC3},
        {.name = "ambient_intensity", .type = ARGUMENT_UNIFORM_FLOAT},
};

char const *spotlight_light_fragment_shader = "";

const struct shader_argument spotlight_light_arguments[] = {};

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
    unused(vertex_arguments);

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
    unused(fragment_arguments);

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
            // the only actually supported light currently
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

    if (backend_program_add_shader(program, vertex_shader, GL_VERTEX_SHADER)) {
        retval = -ERROR_SHADER_COMPILATION_FAILED;
        goto failure;
    }

    if (backend_program_add_shader(
                program, fragment_source, GL_FRAGMENT_SHADER)) {
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
