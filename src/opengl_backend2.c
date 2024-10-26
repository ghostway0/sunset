#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <log.h>

#include "sunset/commands.h"
#include "sunset/config.h"
#include "sunset/errors.h"
#include "sunset/opengl_backend.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

#define SUNSET_MAX_NUM_INSTANCED 128

char const *default_vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 1.0);\n" // projection * view * model *
        "}\n";

char const *default_fragment_shader_source =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";

char const *instanced_vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 transforms[128];\n"
        "void main() {\n"
        "    gl_Position = transforms[gl_InstanceID] * vec4(aPos, 1.0);\n"
        "}\n";

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

static int compile_mesh(
        struct mesh const *mesh, struct compiled_mesh *mesh_out) {
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

void backend_free_program(struct program *program) {
    glDeleteProgram((GLuint)program->handle);
}

uint32_t backend_register_mesh(
        struct render_context *context, struct mesh mesh) {
    struct compiled_mesh compiled_mesh;
    if (compile_mesh(&mesh, &compiled_mesh) != 0) {
        return -1;
    }

    vector_append(context->meshes, compiled_mesh);

    return vector_size(context->meshes) - 1;
}

/*
 * Commands:
    COMMAND_NOP,
    COMMAND_LINE,
    COMMAND_RECT,
    COMMAND_FILLED_RECT,
    COMMAND_ARC,
    COMMAND_FILLED_ARC,
    COMMAND_TEXT,
    COMMAND_IMAGE, (where; tiled?)
    COMMAND_MESH,  (instanced, mesh_id, texture_id)
    COMMAND_CUSTOM (struct program)

enum backend_program_type {
    PROGRAM_DRAW_INSTANCED_MESH,
    PROGRAM_DRAW_MESH,
    PROGRAM_DRAW_DIRECT_LIGHT,
    NUM_BACKEND_PROGRAMS,
};

struct render_context {
    ...

    struct program backend_programs[NUM_BACKEND_PROGRAMS];
    vector(struct compiled_mesh) meshes;
    vector(struct compiled_texture) textures;
};

 * struct instancing_buffer {
 *     uint32_t mesh_id;
 *     vector(mat4) transforms;
 *     vector(uint32_t) required_textures;
 *     GLuint textures_buffer;
 *     GLuint transforms_buffer;
 * }
 *
 * struct frame_cache {
 *    struct instancing_buffer *current_instancing_buffer;
 *    // mesh_id->instancing_buffer
 *    map(struct instancing_buffer) instancing_buffers;
 * }
 *
 * command mesh pseudo-code (instanced, mesh_id, texture_id, transform):
 *   if current_instancing_buffer:
 *     if current_instancing_buffer.mesh_id != mesh_id:
 *       upload_instancing_buffer(current_instancing_buffer);
 *       use_program(context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH]);
 *       draw instanced
 *       resetup
 *     append transform to transform buffer
 *     (?) append texture to texture buffer
 *   elif not instanced:
 *     current_instancing_buffer = NULL;
 *     draw immediately:
 *     use_program(context->backend_programs[PROGRAM_DRAW_MESH]);
 *   else:
 *     if not cache . instance_cache:
 *       allocate in instancing cache (a small transform buffer)
 *     current_instancing_buffer = &cache . instancing_buffers[mesh_id]
 * */

static int setup_default_shaders(struct render_context *context) {
    struct program program;
    if (backend_create_program(&program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(
                &program, default_vertex_shader_source, GL_VERTEX_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(
                &program, default_fragment_shader_source, GL_FRAGMENT_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_link_program(&program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    context->backend_programs[PROGRAM_DRAW_MESH] = program;

    struct program instanced_program;
    if (backend_create_program(&instanced_program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(&instanced_program,
                instanced_vertex_shader_source,
                GL_VERTEX_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(&instanced_program,
                default_fragment_shader_source,
                GL_FRAGMENT_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_link_program(&instanced_program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH] = program;

    return 0;
}

int backend_setup(struct render_context *context, struct render_config config) {
    int retval = 0;

    if (!glfwInit()) {
        return -ERROR_IO;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    char const *title = "Sunset";
    if (config.window_title) {
        title = config.window_title;
    }

    context->window = glfwCreateWindow(
            config.window_width, config.window_height, title, NULL, NULL);

    if (!context->window) {
        retval = -ERROR_IO;
        goto failure;
    }

    glfwMakeContextCurrent(context->window);

    if (glewInit() != GLEW_OK) {
        retval = -ERROR_IO;
        goto failure;
    }

    glEnable(GL_DEPTH_TEST);

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    if ((retval = setup_default_shaders(context))) {
        goto failure;
    }

    vector_init(context->meshes, struct compiled_mesh);

    vector_init(
            context->frame_cache.instancing_buffers, struct instancing_buffer);

    return 0;

failure:
    glfwDestroyWindow(context->window);

    return retval;
}

/*
 * command mesh pseudo-code (instanced, mesh_id, texture_id, transform):
 *   if current_instancing_buffer:
 *     if current_instancing_buffer.mesh_id != mesh_id:
 *       upload_instancing_buffer(current_instancing_buffer);
 *       use_program(context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH]);
 *       draw instanced
 *       resetup
 *     append transform to transform buffer
 *     (?) append texture to texture buffer
 *   elif not instanced:
 *     current_instancing_buffer = NULL;
 *     draw immediately:
 *     use_program(context->backend_programs[PROGRAM_DRAW_MESH]);
 *   else:
 *     if not cache . instance_cache:
 *       allocate in instancing cache (a small transform buffer)
 *     current_instancing_buffer = &cache . instancing_buffers[mesh_id]
 * */

// static int upload_instancing_buffer(struct instancing_buffer *buffer, struct
// program *program) {
//     unused(buffer);
//     unused(program);
//
//     return 0;
// }

static void use_program(struct program program) {
    glUseProgram((GLuint)program.handle);
}

static int program_set_uniform_mat4(struct program program,
        char const *name,
        mat4 const *value,
        size_t num_values) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniformMatrix4fv(loc, num_values, GL_FALSE, (GLfloat *)value);

    return 0;
}

static void draw_instanced_mesh(struct render_context *context,
        uint32_t mesh_id,
        mat4 const *transforms,
        size_t num_transforms,
        uint32_t const *required_textures,
        size_t num_required_textures) {
    unused(required_textures);
    unused(num_required_textures);

    const struct compiled_mesh *mesh = &context->meshes[mesh_id];

    glBindVertexArray(mesh->vao);

    use_program(context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH]);

    // (?) bind textures

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

    program_set_uniform_mat4(
            context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH],
            "transforms",
            transforms,
            num_transforms);

    glDrawElementsInstanced(GL_TRIANGLES,
            mesh->num_indices,
            GL_UNSIGNED_INT,
            0,
            num_transforms);

    glBindVertexArray(0);
}

static uint64_t instancing_buffer_mesh_id(struct instancing_buffer *buffer) {
    return buffer->mesh_id;
}

static void instancing_buffer_flush(
        struct render_context *context, struct instancing_buffer *buffer) {
    // upload_instancing_buffer(buffer,
    // &context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH]);

    vector(mat4) transforms = buffer->transforms;
    vector(uint32_t) required_textures = buffer->required_textures;

    // draw instanced
    draw_instanced_mesh(context,
            buffer->mesh_id,
            transforms,
            vector_size(transforms),
            required_textures,
            vector_size(required_textures));
}

static int run_mesh_command(
        struct render_context *context, struct command_mesh const *command) {
    struct frame_cache *cache = &context->frame_cache;

    if (cache->current_instancing_buffer) {
        if (cache->current_instancing_buffer->mesh_id != command->mesh_id) {
            instancing_buffer_flush(context, cache->current_instancing_buffer);
            vector_clear(cache->current_instancing_buffer->transforms);
            // setup to mesh_id
            cache->current_instancing_buffer =
                    map_get(cache->instancing_buffers,
                            command->mesh_id,
                            instancing_buffer_mesh_id);
        }

        // append transform to transform buffer
        vector_append_copy(cache->current_instancing_buffer->transforms,
                command->transform);

        // (?) append texture to texture buffer
    } else if (!command->instanced) {
        use_program(context->backend_programs[PROGRAM_DRAW_MESH]);

        glBindVertexArray(context->meshes[command->mesh_id].vao);
        glDrawElements(GL_TRIANGLES,
                context->meshes[command->mesh_id].num_indices,
                GL_UNSIGNED_INT,
                0);

        glBindVertexArray(0);
    } else {
        if (!map_get(cache->instancing_buffers,
                    command->mesh_id,
                    instancing_buffer_mesh_id)) {
            struct instancing_buffer buffer;
            buffer.mesh_id = command->mesh_id;
            vector_init(buffer.transforms, mat4);
            vector_init(buffer.required_textures, uint32_t);

            map_insert(cache->instancing_buffers,
                    buffer,
                    instancing_buffer_mesh_id);
        }

        cache->current_instancing_buffer = map_get(cache->instancing_buffers,
                command->mesh_id,
                instancing_buffer_mesh_id);

        vector_append_copy(cache->current_instancing_buffer->transforms,
                command->transform);
    }

    return 0;
}

int backend_flush(struct render_context *context) {
    if (context->frame_cache.current_instancing_buffer) {
        instancing_buffer_flush(
                context, context->frame_cache.current_instancing_buffer);
    }

    for (size_t i = 0; i < vector_size(context->frame_cache.instancing_buffers);
            i++) {
        vector_clear(context->frame_cache.instancing_buffers[i].transforms);
        vector_clear(
                context->frame_cache.instancing_buffers[i].required_textures);
    }

    return 0;
}

void backend_draw(struct render_context *context,
        struct command_buffer *command_buffer,
        mat4 view,
        mat4 projection) {
    unused(view);
    unused(projection);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    while (true) {
        struct command command;
        if (command_buffer_pop(command_buffer, &command)) {
            break;
        }

        if (command.type == COMMAND_NOP) {
            continue;
        }

        switch (command.type) {
            case COMMAND_MESH:
                run_mesh_command(context, &command.data.mesh);
                break;
            default:
                log_error("unhandled command type %d", command.type);
                break;
        }
    }

    backend_flush(context);

    glfwSwapBuffers(context->window);
    glfwPollEvents();
}
