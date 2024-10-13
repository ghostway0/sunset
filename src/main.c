#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <string.h>

#include <log.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/config.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/quadtree.h"
#include "sunset/scene.h"
#include "sunset/utils.h"

#include "daboor.h"

#define container_of(p, T, a)                                                  \
    ((T *)((uintptr_t)(p) - (uintptr_t)(&((T *)(0))->a)))

typedef void (*custom_command)(
        void *render_context, struct command const *command);

struct context {
    struct command_buffer command_buffer;
    struct font *fonts;
    size_t num_fonts;
    void *render_context;
    custom_command custom_commands[MAX_NUM_CUSTOM_COMMANDS];
};

void context_init(struct context *context,
        struct command_buffer_options command_buffer_options,
        struct font *fonts,
        size_t num_fonts,
        void *render_context) {
    context->fonts = fonts;
    context->num_fonts = num_fonts;
    context->render_context = render_context;
    command_buffer_init(&context->command_buffer, command_buffer_options);
    memset(context->custom_commands, 0, sizeof(context->custom_commands));
}

void context_free(struct context *context) {
    command_buffer_free(&context->command_buffer);
}

int stub_render_command(
        struct context *context, struct command const *command) {
    unused(context);

    switch (command->type) {
        case COMMAND_NOP:
            log_info("command_nop");
            break;
        case COMMAND_LINE: {
            struct command_line const *line = &command->data.line;
            log_info("command_line: %d %d %d %d",
                    line->from.x,
                    line->from.y,
                    line->to.x,
                    line->to.y);
            break;
        }
        case COMMAND_RECT: {
            struct command_rect const *rect = &command->data.rect;
            log_info("command_rect: %d %d %d %d",
                    rect->rect.x,
                    rect->rect.y,
                    rect->rect.width,
                    rect->rect.height);
            break;
        }
        case COMMAND_FILLED_RECT: {
            struct command_filled_rect const *filled_rect =
                    &command->data.filled_rect;
            log_info("command_filled_rect: %d %d %d %d",
                    filled_rect->rect.x,
                    filled_rect->rect.y,
                    filled_rect->rect.width,
                    filled_rect->rect.height);
            break;
        }
        case COMMAND_ARC: {
            struct command_arc const *arc = &command->data.arc;
            log_info("command_arc: %d %d %d %f %f",
                    arc->center.x,
                    arc->center.y,
                    arc->r,
                    arc->a0,
                    arc->a1);
            break;
        }
        case COMMAND_FILLED_ARC: {
            struct command_filled_arc const *filled_arc =
                    &command->data.filled_arc;
            log_info("command_filled_arc: %d %d %d %f %f",
                    filled_arc->center.x,
                    filled_arc->center.y,
                    filled_arc->r,
                    filled_arc->a0,
                    filled_arc->a1);
            break;
        }
        case COMMAND_TEXT: {
            struct command_text const *text = &command->data.text;
            log_info("command_text: %d %d %s",
                    text->start.x,
                    text->start.y,
                    text->text);
            break;
        }
        case COMMAND_IMAGE: {
            struct command_image const *image = &command->data.image;
            log_info("command_image: %d %d %zu %zu",
                    image->pos.x,
                    image->pos.y,
                    image->image.w,
                    image->image.h);
            show_image_grayscale_at(&image->image, image->pos);
            break;
        }
        default: {
            size_t custom_command_idx = command->type - NUM_COMMANDS;
            log_info("custom_command %zu", custom_command_idx);
        }
    }

    return 0;
}

int main() {
    int retval = 0;

    struct render_context render_context;

    backend_setup(&render_context, (struct render_config){800, 600});

    struct shader_signature signature = {
            .uniforms = NULL,
            .num_uniforms = 0,
            .ssbos = NULL,
            .num_ssbos = 0,
    };

    FILE *file = fopen("shader.vert", "r");
    if (!file) {
        log_error("fopen failed");
        retval = 1;
        goto cleanup;
    }

    char source[1024];
    size_t num_read = fread(source, 1, sizeof(source), file);
    source[num_read] = '\0';

    struct shader vertex, fragment;

    if (backend_create_shader(source, SHADER_VERTEX, signature, &vertex)) {
        log_error("backend_create_shader failed");
        retval = 1;
        goto cleanup;
    }

    file = fopen("shader.frag", "r");
    if (!file) {
        log_error("fopen failed");
        retval = 1;
        goto cleanup;
    }

    num_read = fread(source, 1, sizeof(source), file);
    source[num_read] = '\0';

    if (backend_create_shader(source, SHADER_FRAGMENT, signature, &fragment)) {
        log_error("backend_create_shader failed");
        retval = 1;
        goto cleanup;
    }

    struct uniform_argument uniform_arguments[] = {};

    struct shader_arguments arguments = {
            .uniforms = uniform_arguments,
            .num_uniforms =
                    sizeof(uniform_arguments) / sizeof(struct uniform_argument),
            .ssbos = NULL,
            .num_ssbos = 0,
    };

    // TODO: should be with a `render_context`
    if (backend_setup_shader(&vertex, &arguments)) {
        log_error("backend_setup_shader failed");
        retval = 1;
        goto cleanup;
    }

    struct byte_stream texture1;
    int texture1_data[] = {0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000};
    byte_stream_from((uint8_t *)texture1_data, 4, &texture1);
    
    struct uniform_argument uniform_arguments2[] = {
    };

    struct shader_arguments arguments2 = {
            .uniforms = uniform_arguments2,
            .num_uniforms = sizeof(uniform_arguments2)
                    / sizeof(struct uniform_argument),
            .ssbos = NULL,
            .num_ssbos = 0,
    };

    if (backend_setup_shader(&fragment, &arguments2)) {
        log_error("backend_setup_shader failed");
        retval = 1;
        goto cleanup;
    }

    float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    unsigned int indices[] = {0, 1, 2};

    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose((GLFWwindow *)render_context.window.handle)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // glUseProgram(vertex.handle);
        // glUseProgram(fragment.handle);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers((GLFWwindow *)render_context.window.handle);
        glfwPollEvents();
    }

cleanup:
    return retval;
}
