#include <stddef.h>
#include <string.h>

#include <log.h>

#include "sunset/commands.h"
#include "sunset/fonts.h"
#include "sunset/gfx.h"
#include "sunset/quadtree.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

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

bool should_split(struct quad_tree *tree, struct quad_node *node) {
    unused(tree);

    struct chunk *chunk = node->data;

    return chunk->num_objects > 5;
}

void *split(struct quad_tree *tree, void *data, struct rect new_bounds) {
    unused(tree);

    struct chunk *chunk = data;

    struct chunk *new_chunk = malloc(sizeof(struct chunk));
    *new_chunk = *chunk;
    new_chunk->bounds = new_bounds;

    size_t in_new_bounds = 0;

    for (size_t i = 0; i < chunk->num_objects; ++i) {
        struct object *object = chunk->objects + i;

        if (position_within_rect(object->position, new_bounds)) {
            in_new_bounds++;
            log_trace("object %zu: " vec3_format " in chunk " rect_format,
                    i,
                    vec3_args(object->position),
                    rect_args(new_bounds));
        }
    }

    log_trace("in_new_bounds: %zu", in_new_bounds);

    new_chunk->num_objects = in_new_bounds;
    new_chunk->objects = malloc(in_new_bounds * sizeof(struct object));

    for (size_t i = 0, j = 0; i < chunk->num_objects; ++i) {
        struct object *object = chunk->objects + i;

        if (position_within_rect(object->position, new_bounds)) {
            new_chunk->objects[j++] = *object;
        }
    }

    chunk->num_objects -= in_new_bounds;

    return new_chunk;
}

int main() {
    int retval = 0;

    struct quad_tree tree;

    struct rect root_bounds = {0, 0, 200, 200};

    struct object *objects = calloc(10, sizeof(struct object));

    struct object pobjects[10] = {
            {.position = {10, 10, 0}},
            {.position = {20, 20, 0}},
            {.position = {30, 30, 0}},
            {.position = {40, 40, 0}},
            {.position = {50, 50, 0}},
            {.position = {110, 10, 0}},
            {.position = {120, 20, 0}},
            {.position = {130, 30, 0}},
            {.position = {140, 40, 0}},
            {.position = {150, 50, 0}},
    };

    memcpy(objects, pobjects, sizeof(pobjects));

    struct chunk root_chunk = {
            .bounds = root_bounds,
            .objects = objects,
            .num_objects = 10,
            .lights = NULL,
            .num_lights = 0,
    };

    quad_tree_create(
            3, 5, should_split, split, NULL, &root_chunk, root_bounds, &tree);

    for (size_t i = 0; i < 10; ++i) {
        struct object *object = objects + i;
        struct chunk *chunk = quad_tree_query(&tree, object->position);

        log_info("object %zu: " vec3_format " in chunk " rect_format,
                i,
                vec3_args(object->position), rect_args(chunk->bounds));
    }

    quad_tree_destroy(&tree);

    return retval;
}
