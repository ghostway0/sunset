#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <string.h>

#include <log.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
// #include "sunset/config.h"
#include "sunset/ecs.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/quadtree.h"
#include "sunset/scene.h"
#include "sunset/utils.h"

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

    struct ecs ecs;
    ecs_init(&ecs);

    {
        struct position position = {3.0f, 4.0f};
        struct entity_builder builder;

        entity_builder_init(&builder, &ecs);
        entity_builder_add_component(&builder, &position, 0);
        entity_builder_finish(&builder);
    }

    {
        struct position position = {1.0f, 2.0f};
        struct velocity velocity = {0.1f, 0.2f};
        struct entity_builder builder;

        entity_builder_init(&builder, &ecs);
        entity_builder_add_component(&builder, &position, 0);
        entity_builder_add_component(&builder, &velocity, 1);
        entity_builder_finish(&builder);
    }

    struct ecs_iterator iterator =
            ecs_iterator_create(&ecs, ECS_COMPONENT(0)); //  | ECS_COMPONENT(1)

    while (ecs_iterator_is_valid(&iterator)) {
        struct position *position =
                ecs_iterator_get_component_raw(&iterator, 0);

        log_info("position: %f %f", position->x, position->y);
        ecs_iterator_advance(&iterator);
    }

    // cleanup:
    return retval;
}
