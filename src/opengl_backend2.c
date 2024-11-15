#include <stddef.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include <cglm/types.h>
#include <log.h>

#include "sunset/bitmap.h"
#include "sunset/commands.h"
#include "sunset/config.h"
#include "sunset/errors.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/map.h"
#include "sunset/opengl_backend.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

#define SUNSET_MAX_NUM_INSTANCED 128

char const *default_vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    TexCoords = aTexCoords;\n"
        "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\n";

char const *default_fragment_shader_source =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    float ndc = (2.0 * gl_FragCoord.z - gl_DepthRange.near - "
        "gl_DepthRange.far) / (gl_DepthRange.far - gl_DepthRange.near);\n"
        "    float clip = ndc / gl_FragCoord.w;\n"
        "    float depth = (clip * 0.5) + 0.5;\n"
        "\n"
        "    float r = sin(depth * 3.1415 * 4.0) * 0.5 + 0.5;\n"
        "    float g = cos(depth * 3.1415 * 4.0) * 0.5 + 0.5;\n"
        "    float b = sin(depth * 3.1415 * 2.0) * 0.5 + 0.5;\n"
        "\n"
        "    FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

char const *textured_fragment_shader_source =
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D texture;\n"
        "void main() {\n"
        "    FragColor = texture(texture, TexCoords);\n"
        "0.0, 1.0);\n"
        "}\n";

char const *instanced_vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 transforms[128];\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    mat4 transform = transforms[gl_InstanceID];\n"
        "    gl_Position = projection * view * transform * vec4(aPos, "
        "1.0);\n"
        "    TexCoords = aTexCoords;\n"
        "}\n";

char const *text_vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec4 aPos;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(aPos.xy, 0.0, 1.0);\n"
        "    TexCoords = aPos.zw;\n"
        "}\n";

char const *text_fragment_shader_source =
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "void main() {\n"
        "    float sampled = texture(text, TexCoords).r;\n"
        "    color = vec4(sampled, sampled, sampled, 1.0);\n"
        "}\n";

static int compile_shader_into(GLuint shader, char const *source) {
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);

        log_error("shader compilation failed: %s", info_log);

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

    context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH] = instanced_program;

    struct program text_program;
    if (backend_create_program(&text_program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(
                &text_program, text_vertex_shader_source, GL_VERTEX_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(&text_program,
                text_fragment_shader_source,
                GL_FRAGMENT_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_link_program(&text_program)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    context->backend_programs[PROGRAM_DRAW_TEXT] = text_program;

    return 0;
}

int backend_setup(struct render_context *context, struct render_config config) {
    int retval = 0;

    if (!glfwInit()) {
        return -ERROR_IO;
    }

    context->width = config.window_width;
    context->height = config.window_height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    if (glewInit() != GLEW_OK) {
        retval = -ERROR_IO;
        goto failure;
    }

    if ((retval = setup_default_shaders(context))) {
        goto failure;
    }

    vector_init(context->meshes, struct compiled_mesh);
    vector_init(
            context->frame_cache.instancing_buffers, struct instancing_buffer);

    glDepthFunc(GL_LESS);

    return 0;

failure:
    glfwDestroyWindow(context->window);

    return retval;
}

// static struct rect get_atlas_region(
//         struct atlas *atlas, size_t width, size_t height) {
//     // resize to the max of w and h
//
//     // return offset into atlas
// }
//
// int compile_texture_into(struct image const *texture, struct atlas *atlas) {
//     ptrdiff_t offset = get_atlas_region(atlas, texture->h, texture->w);
//
//     // write into atlas at offset
//
// }

int backend_register_texture(
        struct render_context *context, struct image const *texture) {
    unused(context);
    unused(texture);

    todo();
}

/// first_id_out gets set the id of the first texture that has been registered
/// in this atlas. the ids are guaranteed to be in-order.
int backend_register_texture_atlas(struct render_context *context,
        struct image const *atlas_image,
        struct rect *bounds,
        size_t num_textures,
        uint32_t *first_id_out) {
    if (vector_size(context->textures) >= (uint32_t)-1) {
        return ERROR_BACKEND_UNKNOWN;
    }

    // TODO: check for opengl errors

    GLuint atlas;
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);

    // TODO: support non-rgba images
    glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_RGBA,
            atlas_image->w,
            atlas_image->h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            atlas_image->pixels);

    GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    vector_append(context->atlases, (struct atlas){.buffer = atlas});

    uint32_t atlas_id = vector_size(context->atlases) - 1;
    *first_id_out = vector_size(context->textures);

    for (size_t i = 0; i < num_textures; i++) {
        struct compiled_texture compiled = {
                .atlas_id = atlas_id,
                .bounds = bounds[i],
        };

        vector_append(context->textures, compiled);
    }

    return 0;
}

GLint compile_texture(struct image const *atlas_image) {
    GLuint atlas;
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);

    // TODO: support non-rgba images
    glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_RGBA,
            atlas_image->w,
            atlas_image->h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            atlas_image->pixels);

    GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return atlas;
}

void backend_set_user_context(
        struct render_context *context, void *user_context) {
    glfwSetWindowUserPointer(context->window, user_context);
}

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

[[maybe_unused]]
static int program_set_uniform_int(
        struct program program, char const *name, int value) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniform1i(loc, value);

    return 0;
}

[[maybe_unused]]
static int program_set_uniform_float(
        struct program program, char const *name, float value) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniform1f(loc, value);

    return 0;
}

[[maybe_unused]]
static int program_set_uniform_vec3(
        struct program program, char const *name, vec3 const *value) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniform3fv(loc, 1, (GLfloat *)value);

    return 0;
}

static void upload_default_uniforms(
        struct render_context *context, struct program program) {
    program_set_uniform_mat4(
            program, "model", &context->frame_cache.model_matrix, 1);
    program_set_uniform_mat4(
            program, "view", &context->frame_cache.view_matrix, 1);
    program_set_uniform_mat4(
            program, "projection", &context->frame_cache.projection_matrix, 1);

    // (?) bind textures
}

[[maybe_unused]]
static int upload_instancing_buffer(
        struct instancing_buffer *buffer, struct program program) {
    if (program_set_uniform_mat4(program,
                "transforms",
                buffer->transforms,
                vector_size(buffer->transforms))) {
        return -ERROR_SHADER_LOADING_FAILED;
    }

    return 0;
}

static void draw_instanced_mesh(struct render_context *context,
        uint32_t mesh_id,
        mat4 const *transforms,
        size_t num_transforms) {
    const struct compiled_mesh *mesh = &context->meshes[mesh_id];
    struct program program =
            context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH];

    glBindVertexArray(mesh->vao);

    // (?) bind textures

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

    use_program(program);

    // upload_default_uniforms(context, program);

    program_set_uniform_mat4(
            program, "view", &context->frame_cache.view_matrix, 1);
    program_set_uniform_mat4(
            program, "projection", &context->frame_cache.projection_matrix, 1);

    program_set_uniform_mat4(program, "transforms", transforms, num_transforms);

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
    vector(mat4) transforms = buffer->transforms;

    // draw instanced
    draw_instanced_mesh(
            context, buffer->mesh_id, transforms, vector_size(transforms));

    vector_clear(transforms);
}

static int run_mesh_command(
        struct render_context *context, struct command_mesh command) {
    struct frame_cache *cache = &context->frame_cache;

    if (!command.instanced) {
        struct program program = context->backend_programs[PROGRAM_DRAW_MESH];
        struct compiled_mesh *mesh = &context->meshes[command.mesh_id];

        glm_mat4_copy(command.transform, cache->model_matrix);

        glBindVertexArray(mesh->vao);

        // (?) bind texture

        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

        use_program(program);

        upload_default_uniforms(context, program);

        glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    } else {
        if (!map_get(cache->instancing_buffers,
                    command.mesh_id,
                    instancing_buffer_mesh_id)) {
            struct instancing_buffer buffer;
            buffer.mesh_id = command.mesh_id;
            vector_init(buffer.transforms, mat4);

            map_insert(cache->instancing_buffers,
                    buffer,
                    instancing_buffer_mesh_id);
        }

        struct instancing_buffer *buffer = map_get(cache->instancing_buffers,
                command.mesh_id,
                instancing_buffer_mesh_id);

        vector_append_copy(buffer->transforms, command.transform);
    }

    return 0;
}

int backend_start_frame(
        struct render_context *context, mat4 view, mat4 projection) {
    glm_mat4_copy(projection, context->frame_cache.projection_matrix);
    glm_mat4_copy(view, context->frame_cache.view_matrix);

    return 0;
}

int backend_flush(struct render_context *context) {
    for (size_t i = 0; i < vector_size(context->frame_cache.instancing_buffers);
            ++i) {
        instancing_buffer_flush(
                context, &context->frame_cache.instancing_buffers[i]);
    }

    return 0;
}

static int run_text_command(
        struct render_context *context, struct command_text command) {
    struct program program = context->backend_programs[PROGRAM_DRAW_TEXT];

    use_program(program);

    mat4 ortho_projection;
    glm_ortho(0.0f,
            context->width,
            0.0f,
            context->height,
            -1.0f,
            1.0f,
            ortho_projection);

    program_set_uniform_mat4(program, "projection", &ortho_projection, 1);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    float scale = 1;

    float y = command.alignment == WINDOW_POINT_TOP_LEFT
                    || command.alignment == WINDOW_POINT_TOP_RIGHT
            ? context->height - command.start.y
            : command.start.y;

    float current_x = command.start.x;

    for (size_t i = 0; i < command.text_len; i++) {
        struct glyph const *glyph =
                font_get_glyph(command.font, command.text[i]);

        if (!glyph) {
            continue;
        }

        GLuint tex;
        if ((tex = compile_texture(&glyph->image)) == 0) {
            return -1;
        }

        float xpos = current_x + glyph->bounds.x * scale;
        float ypos = y + glyph->bounds.y * scale;
        float w = glyph->bounds.width * scale;
        float h = glyph->bounds.height * scale;

        float vertices[6][4] = {{xpos, ypos + h, 0.0f, 1.0f},
                {xpos, ypos, 0.0f, 0.0f},
                {xpos + w, ypos, 1.0f, 0.0f},

                {xpos, ypos + h, 0.0f, 1.0f},
                {xpos + w, ypos, 1.0f, 0.0f},
                {xpos + w, ypos + h, 1.0f, 1.0f}};

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        program_set_uniform_int(program, "text", 0);

        vec3 color = {1.0f, 1.0f, 1.0f};
        program_set_uniform_vec3(program, "textColor", &color);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        current_x += glyph->advance_x * scale;

        glDeleteTextures(1, &tex);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    return 0;
}

[[maybe_unused]]
int backend_regsiter_texture(
        struct render_context *context, struct texture const *texture) {
    unused(context);
    unused(texture);

    return 0;
}

void backend_draw(struct render_context *context,
        struct command_buffer *command_buffer,
        mat4 view,
        mat4 projection) {
    backend_start_frame(context, view, projection);

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
                run_mesh_command(context, command.data.mesh);
                break;
            case COMMAND_TEXT:
                run_text_command(context, command.data.text);
                break;
            case COMMAND_SET_ZINDEX:
                size_t zindex = command.data.set_zindex.zindex;
                float depth_offset = zindex * 0.1;
                glDepthRange(0.0 + depth_offset, 1.0 - depth_offset);

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
