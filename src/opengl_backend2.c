#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include <cglm/types.h>
#include <log.h>

#include "internal/math.h"
#include "internal/mem_utils.h"
#include "internal/utils.h"
#include "obj_file.h"
#include "sunset/bitmask.h"
#include "sunset/commands.h"
#include "sunset/config.h"
#include "sunset/errors.h"
#include "sunset/events.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/input.h"
#include "sunset/map.h"
#include "sunset/opengl_backend.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/vector.h"

#define SUNSET_MAX_NUM_INSTANCED 128

struct program_config {
    char const *vertex;
    char const *fragment;
};

char const default_vertex_shader_source[] =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform vec4 bounds;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPos, "
        "1.0);\n"
        "    TexCoords = aTexCoords * bounds.zw + bounds.xy;\n"
        "}\n";

char const textured_fragment_shader_source[] =
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D sampler;\n"
        "void main() {\n"
        "    FragColor = texture(sampler, TexCoords);\n"
        "}\n";

const struct program_config default_program_config = {
        .vertex = default_vertex_shader_source,
        .fragment =
                "#version 330 core\n"
                "out vec4 FragColor;\n"
                "void main() {\n"
                "    float ndc = (2.0 * gl_FragCoord.z - "
                "gl_DepthRange.near - "
                "gl_DepthRange.far) / (gl_DepthRange.far - "
                "gl_DepthRange.near);\n"
                "    float clip = ndc / gl_FragCoord.w;\n"
                "    float depth = (clip * 0.5) + 0.5;\n"
                "\n"
                "    float r = sin(depth * 3.1415 * 4.0) * 0.5 + 0.5;\n"
                "    float g = cos(depth * 3.1415 * 4.0) * 0.5 + 0.5;\n"
                "    float b = sin(depth * 3.1415 * 2.0) * 0.5 + 0.5;\n"
                "\n"
                "    FragColor = vec4(r, g, b, 1.0);\n"
                "}\n",
};

const struct program_config direct_program_config = {
        .vertex =
                "#version 330 core\n"
                "layout (location = 0) in vec2 aPos;\n"
                "layout (location = 1) in vec2 aTexCoords;\n"
                "out vec2 TexCoords;\n"
                "uniform mat4 projection;\n"
                "void main() {\n"
                "    gl_Position = projection * vec4(aPos.xy, 0.0, 1.0);\n"
                "    TexCoords = aTexCoords;\n"
                "}\n",

        .fragment =
                "#version 330 core\n"
                "in vec2 TexCoords;\n"
                "out vec4 FragColor;\n"
                "uniform sampler2D sampler;\n"
                "void main() {\n"
                "    FragColor = texture(sampler, TexCoords);\n"
                "}\n",
};

const struct program_config textured_program_config = {
        .vertex = default_vertex_shader_source,
        .fragment = textured_fragment_shader_source,
};

const struct program_config instanced_textured_program_config = {
        .vertex =
                "#version 330 core\n"
                "layout (location = 0) in vec3 aPos;\n"
                "layout (location = 1) in vec2 aTexCoords;\n"
                "out vec2 TexCoords;\n"
                "uniform mat4 transforms[128];\n"
                "uniform mat4 view;\n"
                "uniform mat4 projection;\n"
                "uniform vec4 bounds;\n"
                "void main() {\n"
                "    mat4 transform = transforms[gl_InstanceID];\n"
                "    gl_Position = projection * view * transform * "
                "vec4(aPos, 1.0);\n"
                "   TexCoords = aTexCoords * bounds.zw + bounds.xy;\n"
                "}\n",
        .fragment = textured_fragment_shader_source,
};

const struct program_config text_program_config = {
        .vertex =
                "#version 330 core\n"
                "layout (location = 0) in vec4 aPos;\n"
                "out vec2 TexCoords;\n"
                "uniform mat4 projection;\n"
                "void main() {\n"
                "    gl_Position = projection * vec4(aPos.xy, 0.0, 1.0);\n"
                "    TexCoords = aPos.zw;\n"
                "}\n",

        .fragment =
                "#version 330 core\n"
                "in vec2 TexCoords;\n"
                "out vec4 color;\n"
                "uniform sampler2D text;\n"
                "void main() {\n"
                "    float sampled = texture(text, TexCoords).r;\n"
                "    color = vec4(sampled, sampled, sampled, 1.0);\n"
                "}\n",
};

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

static int compile_model(
        Model const *model, struct compiled_mesh *mesh_out) {
    glGenVertexArrays(1, &mesh_out->vao);
    glGenBuffers(1, &mesh_out->vbo);
    glGenBuffers(1, &mesh_out->ebo);
    glGenBuffers(1, &mesh_out->tbo);

    glBindVertexArray(mesh_out->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh_out->vbo);
    glBufferData(GL_ARRAY_BUFFER,
            vector_size(model->vertices) * sizeof(vec3),
            model->vertices,
            GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mesh_out->tbo);
    glBufferData(GL_ARRAY_BUFFER,
            vector_size(model->texcoords) * sizeof(vec2),
            model->texcoords,
            GL_STATIC_DRAW);

    size_t total_indices = 0;
    for (size_t i = 0; i < vector_size(model->faces); i++) {
        total_indices += (vector_size(model->faces[i]) - 2) * 3;
    }

    uint32_t *ibo =
            (uint32_t *)sunset_malloc(total_indices * sizeof(uint32_t));

    size_t index = 0;
    for (size_t i = 0; i < vector_size(model->faces); i++) {
        vector(FaceElement) face = model->faces[i];

        for (size_t j = 2; j < vector_size(face); j++) {
            ibo[index++] = face[0].vertex_index;
            ibo[index++] = face[j - 1].vertex_index;
            ibo[index++] = face[j].vertex_index;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_out->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            total_indices * sizeof(uint32_t),
            ibo,
            GL_STATIC_DRAW);

    free(ibo);

    // position
    glBindBuffer(GL_ARRAY_BUFFER, mesh_out->vbo);
    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // texture
    glBindBuffer(GL_ARRAY_BUFFER, mesh_out->tbo);
    glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    mesh_out->num_indices = total_indices;

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

void backend_destroy_program(struct program *program) {
    glDeleteProgram((GLuint)program->handle);
}

uint32_t backend_register_mesh(RenderContext *context, Model mesh) {
    struct compiled_mesh compiled_mesh;
    if (compile_model(&mesh, &compiled_mesh) != 0) {
        return -1;
    }

    vector_append(context->meshes, compiled_mesh);

    return vector_size(context->meshes) - 1;
}

static int add_preconfigured_shader(

        struct program_config config, struct program *program_out) {
    if (backend_create_program(program_out)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(
                program_out, config.vertex, GL_VERTEX_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_program_add_shader(
                program_out, config.fragment, GL_FRAGMENT_SHADER)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    if (backend_link_program(program_out)) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    return 0;
}

static int setup_default_shaders(RenderContext *context) {
    int retval = 0;

    if ((retval = add_preconfigured_shader(textured_program_config,
                 &context->backend_programs[PROGRAM_DRAW_MESH]))) {
        return retval;
    }

    if ((retval = add_preconfigured_shader(default_program_config,
                 &context->backend_programs[PROGRAM_DEFAULT_MESH]))) {
        return retval;
    }

    if ((retval = add_preconfigured_shader(
                 instanced_textured_program_config,
                 &context->backend_programs
                         [PROGRAM_DRAW_INSTANCED_MESH]))) {
        return retval;
    }

    if ((retval = add_preconfigured_shader(text_program_config,
                 &context->backend_programs[PROGRAM_DRAW_TEXT]))) {
        return retval;
    }

    if ((retval = add_preconfigured_shader(direct_program_config,
                 &context->backend_programs[PROGRAM_DRAW_DIRECT]))) {
        return retval;
    }

    return 0;
}

static Key sys_to_key(int key, int mods) {
    unused(mods);

    if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST) {
        return (Key)key;
    }

    return HIGHEST_KEY;
}

static void key_callback(
        GLFWwindow *window, int key, int scancode, int action, int mods) {
    unused(scancode);

    RenderContext *context = glfwGetWindowUserPointer(window);
    pthread_mutex_lock(&context->lock);

    Key app_key = sys_to_key(key, mods);

    if (app_key == HIGHEST_KEY) {
        goto cleanup;
    }

    if (action == GLFW_PRESS) {
        bitmask_set(&context->keyboard_state, app_key);

        events_push(
                context->event_queue, SYSTEM_EVENT_KEY_PRESSED, app_key);
    } else if (action == GLFW_REPEAT) {
        events_push(context->event_queue, SYSTEM_EVENT_KEY_REPEAT, app_key);
    } else if (action == GLFW_RELEASE) {
        bitmask_unset(&context->keyboard_state, app_key);
    }

cleanup:
    pthread_mutex_unlock(&context->lock);
}

static void mouse_move_callback(
        GLFWwindow *window, double xpos, double ypos) {
    RenderContext *context = glfwGetWindowUserPointer(window);

    pthread_mutex_lock(&context->lock);

    if (context->first_mouse) {
        context->mouse = (Point){xpos, ypos};
        context->first_mouse = false;
        pthread_mutex_unlock(&context->lock);
        return;
    }

    events_push(context->event_queue,
            SYSTEM_EVENT_MOUSE_MOVE,
            (MouseMoveEvent){
                    .offset = {context->mouse.x - xpos,
                            context->mouse.y - ypos},
                    .absolute = {xpos, ypos},
                    .mouse_buttons = context->mouse_buttons,
            });

    context->mouse = (Point){xpos, ypos};

    pthread_mutex_unlock(&context->lock);
}

size_t backend_get_click_id(RenderContext *context) {
    pthread_mutex_lock(&context->lock);
    size_t click_id = context->click_id;
    pthread_mutex_unlock(&context->lock);
    return click_id + 1;
}

static MouseButton sys_to_mouse_button(int button) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MOUSE_BUTTON_LEFT;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MOUSE_BUTTON_RIGHT;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MOUSE_BUTTON_MIDDLE;
        default:
            return MOUSE_BUTTON_UNKNOWN;
    }
}

static void mouse_button_callback(
        GLFWwindow *window, int button, int action, int mods) {
    unused(mods);

    RenderContext *context = glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        bitmask_set(&context->mouse_buttons, sys_to_mouse_button(button));

        events_push(context->event_queue,
                SYSTEM_EVENT_MOUSE_CLICK,
                (MouseClickEvent){
                        sys_to_mouse_button(button), ++context->click_id});
    } else if (action == GLFW_RELEASE) {
        bitmask_unset(&context->mouse_buttons, sys_to_mouse_button(button));
    }
}

static int setup_mouse(RenderContext *context) {
    glfwSetCursorPosCallback(context->window, mouse_move_callback);
    glfwSetMouseButtonCallback(context->window, mouse_button_callback);

    return 0;
}

static void framebuffer_size_callback(
        GLFWwindow *window, int width, int height) {
    RenderContext *render_context = glfwGetWindowUserPointer(window);

    pthread_mutex_lock(&render_context->lock);

    glViewport(0, 0, width, height);

    render_context->first_mouse = true;
    render_context->screen_width = width;
    render_context->screen_height = height;

    glm_ortho(0.0f,
            render_context->screen_width,
            0.0f,
            render_context->screen_height,
            -1.0f,
            1.0f,
            render_context->ortho_projection);

    events_push(render_context->event_queue,
            SYSTEM_EVENT_VIEWPORT_CHANGED,
            (Point){.x = width, .y = height});

    pthread_mutex_unlock(&render_context->lock);
}

int backend_setup(RenderContext *context,
        EventQueue *event_queue,
        RenderConfig config) {
    int retval = 0;

    if (!glfwInit()) {
        return -ERROR_IO;
    }

    context->screen_width = config.window_width;
    context->screen_height = config.window_height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    char const *title =
            config.window_title != NULL ? config.window_title : "Sunset";

    context->window = glfwCreateWindow(
            config.window_width, config.window_height, title, NULL, NULL);

    if (!context->window) {
        retval = -ERROR_IO;
        goto failure;
    }

    glfwMakeContextCurrent(context->window);

    if (config.enable_vsync) {
        glfwSwapInterval(1);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    if (glewInit() != GLEW_OK) {
        retval = -ERROR_IO;
        goto failure;
    }

    context->event_queue = event_queue;

    if ((retval = setup_default_shaders(context))) {
        goto failure;
    }

    if ((retval = setup_mouse(context))) {
        goto failure;
    }

    glfwSetWindowUserPointer(context->window, context);

    glfwSetFramebufferSizeCallback(
            context->window, framebuffer_size_callback);

    vector_init(context->meshes);
    vector_init(context->frame_cache.instancing_buffers);
    vector_init(context->textures);
    vector_init(context->atlases);

    pthread_mutex_init(&context->lock, NULL);

    bitmask_init_empty(NUM_MOUSE_BUTTONS, &context->mouse_buttons);
    bitmask_init_empty(HIGHEST_KEY, &context->keyboard_state);
    bitmask_init_empty(HIGHEST_KEY, &context->state_temp);

    glfwSetKeyCallback(context->window, key_callback);

    glm_ortho(0.0f,
            context->screen_width,
            0.0f,
            context->screen_height,
            -1.0f,
            1.0f,
            context->ortho_projection);

    glDepthFunc(GL_LESS);

    return 0;

failure:
    glfwDestroyWindow(context->window);

    return retval;
}

void backend_generate_input_events(RenderContext *context) {
    bitmask_copy(&context->state_temp, &context->keyboard_state);

    while (!bitmask_is_zero(&context->state_temp)) {
        size_t key = bitmask_ctz(&context->state_temp);
        events_push(context->event_queue, SYSTEM_EVENT_KEY_DOWN, key);
        bitmask_unset(&context->state_temp, key);
    }
}

// static Rect get_atlas_region(
//         struct atlas *atlas, size_t width, size_t height) {
//     // resize to the max of w and h
//
//     // return offset into atlas
// }
//
// int compile_texture_into(Image const *texture, struct atlas
// *atlas) {
//     ptrdiff_t offset = get_atlas_region(atlas, texture->h, texture->w);
//
//     // write into atlas at offset
//
// }

/// first_id_out gets set the id of the first texture that has been
/// registered in this atlas. the ids are guaranteed to be in-order.
int backend_register_texture_atlas(RenderContext *context,
        Image const *atlas_image,
        Rect *bounds,
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    vector_append(context->atlases,
            (struct atlas){.buffer = atlas,
                    .size = {atlas_image->w, atlas_image->h}});

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

int backend_register_texture(
        RenderContext *context, Image const *texture, uint32_t *id_out) {
    Rect bounds[] = {{0, 0, texture->w, texture->h}};
    return backend_register_texture_atlas(
            context, texture, bounds, 1, id_out);
}

GLint compile_texture(Image const *atlas_image) {
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

static int program_set_uniform_vec3(
        struct program program, char const *name, vec3 const *value) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniform3fv(loc, 1, (GLfloat *)value);

    return 0;
}

static int program_set_uniform_vec4(
        struct program program, char const *name, vec4 value) {
    GLint loc = glGetUniformLocation((GLuint)program.handle, name);
    if (loc == -1) {
        return -ERROR_SHADER_COMPILATION_FAILED;
    }

    glUniform4fv(loc, 1, (GLfloat *)value);

    return 0;
}

static void upload_default_uniforms(
        RenderContext *context, struct program program) {
    program_set_uniform_mat4(
            program, "model", &context->frame_cache.model_matrix, 1);
    program_set_uniform_mat4(
            program, "view", &context->frame_cache.view_matrix, 1);
    program_set_uniform_mat4(program,
            "projection",
            &context->frame_cache.projection_matrix,
            1);
}

static void draw_instanced_mesh(RenderContext *context,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 const *transforms,
        size_t num_transforms) {
    const struct compiled_mesh *mesh = &context->meshes[mesh_id];
    struct program program =
            context->backend_programs[PROGRAM_DRAW_INSTANCED_MESH];

    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->tbo);

    use_program(program);

    program_set_uniform_mat4(
            program, "view", &context->frame_cache.view_matrix, 1);
    program_set_uniform_mat4(program,
            "projection",
            &context->frame_cache.projection_matrix,
            1);

    program_set_uniform_mat4(
            program, "transforms", transforms, num_transforms);

    program_set_uniform_int(program, "sampler", 0);

    struct compiled_texture t = context->textures[texture_id];

    struct atlas atlas = context->atlases[t.atlas_id];
    glBindTexture(GL_TEXTURE_2D, atlas.buffer);

    vec4 bounds = {
            (t.bounds.x / atlas.size[0]) * 2 - 1,
            (t.bounds.y / atlas.size[1]) * 2 - 1,
            (t.bounds.w / atlas.size[0]) * 2 - 1,
            (t.bounds.h / atlas.size[1]) * 2 - 1,
    };
    program_set_uniform_vec4(program, "bounds", bounds);

    glDrawElementsInstanced(GL_TRIANGLES,
            mesh->num_indices,
            GL_UNSIGNED_INT,
            0,
            num_transforms);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

static Order compare_instancing_buffers(void const *a, void const *b) {
    struct instancing_buffer *a_data = (struct instancing_buffer *)a;
    struct instancing_buffer *b_data = (struct instancing_buffer *)b;

    if (a_data->mesh_id > b_data->mesh_id) {
        return ORDER_GREATER_THAN;
    }

    if (a_data->mesh_id < b_data->mesh_id) {
        return ORDER_LESS_THAN;
    }

    return ORDER_EQUAL;
}

static void instancing_buffer_flush(
        RenderContext *context, struct instancing_buffer *buffer) {
    vector(mat4) transforms = buffer->transforms;

    // draw instanced
    draw_instanced_mesh(context,
            buffer->mesh_id,
            buffer->atlas_id,
            transforms,
            vector_size(transforms));

    vector_clear(transforms);
}

static int run_mesh_command(RenderContext *context, CommandMesh command) {
    struct frame_cache *cache = &context->frame_cache;

    if (command.texture_id == UINT32_MAX) {
        struct program program =
                context->backend_programs[PROGRAM_DEFAULT_MESH];
        struct compiled_mesh *mesh = &context->meshes[command.mesh_id];

        glBindVertexArray(mesh->vao);

        glBindBuffer(GL_ARRAY_BUFFER, mesh->tbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

        use_program(program);

        program_set_uniform_mat4(
                program, "model", &context->current_context->model, 1);
        program_set_uniform_mat4(
                program, "view", &context->frame_cache.view_matrix, 1);
        program_set_uniform_mat4(program,
                "projection",
                &context->frame_cache.projection_matrix,
                1);

        glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    } else if (!command.instanced) {
        struct program program =
                context->backend_programs[PROGRAM_DRAW_MESH];
        struct compiled_mesh *mesh = &context->meshes[command.mesh_id];

        glm_mat4_copy(context->current_context->model, cache->model_matrix);

        glBindVertexArray(mesh->vao);

        glBindBuffer(GL_ARRAY_BUFFER, mesh->tbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

        use_program(program);

        struct compiled_texture t = context->textures[command.texture_id];

        program_set_uniform_vec4(program,
                "bounds",
                (vec4){t.bounds.x, t.bounds.y, t.bounds.w, t.bounds.h});

        program_set_uniform_int(program, "sampler", 0);

        glBindTexture(GL_TEXTURE_2D, context->atlases[t.atlas_id].buffer);

        upload_default_uniforms(context, program);

        glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    } else {
        if (command.texture_id >= vector_size(context->textures)) {
            return ERROR_OUT_OF_BOUNDS;
        }

        struct compiled_texture texture =
                context->textures[command.texture_id];

        if (!map_get(cache->instancing_buffers,
                    command.mesh_id,
                    compare_instancing_buffers)) {
            struct instancing_buffer buffer;
            buffer.mesh_id = command.mesh_id;
            buffer.atlas_id = texture.atlas_id;
            vector_init(buffer.transforms);

            map_insert(cache->instancing_buffers,
                    buffer,
                    compare_instancing_buffers);
        }

        struct instancing_buffer *buffer =
                map_get(cache->instancing_buffers,
                        command.mesh_id,
                        compare_instancing_buffers);

        assert(texture.atlas_id == buffer->atlas_id &&
               "it is currently required that anything that gets "
               "instanced together is stored within a single atlas");

        vector_append_copy(
                buffer->transforms, context->current_context->model);
    }

    return 0;
}

int backend_start_frame(
        RenderContext *context, mat4 view, mat4 projection) {
    glm_mat4_copy(projection, context->frame_cache.projection_matrix);
    glm_mat4_copy(view, context->frame_cache.view_matrix);

    return 0;
}

int backend_flush(RenderContext *context) {
    for (size_t i = 0;
            i < vector_size(context->frame_cache.instancing_buffers);
            ++i) {
        instancing_buffer_flush(
                context, &context->frame_cache.instancing_buffers[i]);
    }

    return 0;
}

// HACK:
static int run_text_command(RenderContext *context, CommandText command) {
    struct program program = context->backend_programs[PROGRAM_DRAW_TEXT];

    use_program(program);

    program_set_uniform_mat4(
            program, "projection", &context->ortho_projection, 1);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
            GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    float y = command.origin == WINDOW_TOP_LEFT
                    || command.origin == WINDOW_TOP_RIGHT
            ? context->screen_height - command.start.y
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

        float scale = (float)command.size / (float)glyph->image.h;

        float xpos = current_x + glyph->bounds.x * scale;
        float ypos = y - (glyph->bounds.h + glyph->bounds.y) * scale;
        float w = glyph->bounds.w * scale;
        float h = glyph->bounds.h * scale;

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

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    return 0;
}

bool backend_should_stop(RenderContext *context) {
    return glfwWindowShouldClose(context->window);
}

static void run_rect_command(RenderContext *context, CommandRect command) {
    Color color = command.color;
    Rect rect = command.bounds;

    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;

    // FIXME: make this actually accurate
    rect.x = command.origin == WINDOW_TOP_RIGHT
                    || command.origin == WINDOW_BOTTOM_RIGHT
            ? context->screen_width - rect.x
            : rect.x;
    rect.y = command.origin == WINDOW_TOP_RIGHT
                    || command.origin == WINDOW_TOP_LEFT
            ? context->screen_height - rect.y
            : rect.y;

    float x1 = ((float)rect.x / (float)context->screen_width) * 2.0f - 1.0f;
    float y1 =
            ((float)rect.y / (float)context->screen_height) * 2.0f - 1.0f;
    float x2 =
            ((float)(rect.x + rect.w) / (float)context->screen_width) * 2.0f
            - 1.0f;
    float y2 = ((float)(rect.y - rect.h) / (float)context->screen_height)
                    * 2.0f
            - 1.0f;

    struct program program = context->backend_programs[PROGRAM_DRAW_DIRECT];
    use_program(program);

    float vertices[8][3] = {
            {x1, y1, 0.0f},
            {x2, y1, 0.0f},
            {x2, y1, 0.0f},
            {x2, y2, 0.0f},
            {x2, y2, 0.0f},
            {x1, y2, 0.0f},
            {x1, y2, 0.0f},
            {x1, y1, 0.0f},
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
            GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // TODO: this should be automated somehow
    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    program_set_uniform_vec4(program, "color", (vec4){r, g, b, a});

    glDrawArrays(GL_LINES, 0, 8);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

static void run_fill_rect_command(
        RenderContext *context, CommandFilledRect command) {
    Rect rect = command.rect;
    Color color = command.color;

    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;

    // FIXME: make this actually accurate
    rect.x = command.origin == WINDOW_TOP_RIGHT
                    || command.origin == WINDOW_BOTTOM_RIGHT
            ? context->screen_width - rect.x
            : rect.x;
    rect.y = command.origin == WINDOW_TOP_RIGHT
                    || command.origin == WINDOW_TOP_LEFT
            ? context->screen_height - rect.y
            : rect.y;

    float x1 = (rect.x / (float)context->screen_width) * 2.0f - 1.0f;
    float y1 = (rect.y / (float)context->screen_height) * 2.0f - 1.0f;
    float x2 = ((rect.x + rect.w) / (float)context->screen_width) * 2.0f
            - 1.0f;
    float y2 = ((rect.y - rect.h) / (float)context->screen_height) * 2.0f
            - 1.0f;

    struct program program = context->backend_programs[PROGRAM_DRAW_DIRECT];
    use_program(program);

    float vertices[6][3] = {
            {x1, y1, 0.0f},
            {x2, y1, 0.0f},
            {x1, y2, 0.0f},
            {x1, y2, 0.0f},
            {x2, y1, 0.0f},
            {x2, y2, 0.0f},
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
            GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    program_set_uniform_vec4(program, "color", (vec4){r, g, b, a});

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void run_image_command(RenderContext *context, CommandImage command) {
    Image sliced;
    if (image_slice(&command.image, command.bounds, &sliced)) {
        return;
    }

    GLuint i = compile_texture(&sliced);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(
            0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,
            2,
            GL_FLOAT,
            GL_FALSE,
            4 * sizeof(float),
            (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    struct program program = context->backend_programs[PROGRAM_DRAW_DIRECT];
    use_program(program);

    glBindTexture(GL_TEXTURE_2D, i);

    float xpos = command.pos.x;
    float ypos = command.pos.y;
    float h = command.image.h;
    float w = command.image.w;

    float vertices[6][4] = {{xpos, ypos + h, 0.0f, 1.0f},
            {xpos, ypos, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 0.0f},

            {xpos, ypos + h, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 0.0f},
            {xpos + w, ypos + h, 1.0f, 1.0f}};

    glBufferData(
            GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    program_set_uniform_mat4(
            program, "projection", &context->ortho_projection, 1);
    program_set_uniform_int(program, "sampler", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void backend_draw(RenderContext *context,
        CommandBuffer *command_buffer,
        mat4 view,
        mat4 projection) {
    backend_start_frame(context, view, projection);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    while (true) {
        Command command;
        if (cmdbuf_pop(command_buffer, &command)) {
            break;
        }

        if (command.type == COMMAND_NOP) {
            continue;
        }

        switch (command.type) {
            case COMMAND_NOP:
                break;
            case COMMAND_FILLED_RECT:
                run_fill_rect_command(context, command.filled_rect);
                break;
            case COMMAND_RECT:
                run_rect_command(context, command.rect);
                break;
            case COMMAND_MESH:
                run_mesh_command(context, command.mesh);
                break;
            case COMMAND_TEXT:
                run_text_command(context, command.text);
                break;
            case COMMAND_SET_ZINDEX: {
                size_t zindex = command.set_zindex.zindex;
                float depth_offset = zindex * 0.1;
                glDepthRange(0.0 + depth_offset, 1.0 - depth_offset);
                break;
            }
            case COMMAND_SET_CONTEXT:
                context->current_context = command.set_context.context;
                break;
            case COMMAND_IMAGE:
                run_image_command(context, command.image);
                break;
            default:
                todo();
                break;
        }
    }

    backend_flush(context);

    glfwSwapBuffers(context->window);
    glfwPollEvents();
}

void backend_hide_mouse(RenderContext *context) {
    glfwSetInputMode(context->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void backend_show_mouse(RenderContext *context) {
    glfwSetInputMode(context->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void program_destroy(struct program *program) {
    if (!program->handle) {
        return;
    }

    glDeleteProgram(program->handle);
}

void backend_destroy(RenderContext *context) {
    for (size_t i = 0; i < NUM_BACKEND_PROGRAMS; i++) {
        program_destroy(&context->backend_programs[i]);
    }

    vector_destroy(context->meshes);
    vector_destroy(context->atlases);
    vector_destroy(context->textures);
}
