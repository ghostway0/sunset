#include <stddef.h>
#include <string.h>

#include <log.h>

#include "sunset/commands.h"
#include "sunset/errors.h"
#include "sunset/fonts.h"
#include "sunset/gfx.h"
#include "sunset/utils.h"

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
                    rect->rect.pos.x,
                    rect->rect.pos.y,
                    rect->rect.w,
                    rect->rect.h);
            break;
        }
        case COMMAND_FILLED_RECT: {
            struct command_filled_rect const *filled_rect =
                    &command->data.filled_rect;
            log_info("command_filled_rect: %d %d %d %d",
                    filled_rect->rect.pos.x,
                    filled_rect->rect.pos.y,
                    filled_rect->rect.w,
                    filled_rect->rect.h);
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
    struct context context;
    struct font font;
    int retval = 0;

    if ((retval = load_font_psf2("Tamsyn6x12b.psf", "test", &font))) {
        error_print("load_font_psf2", retval);
        goto cleanup;
    }

    struct glyph const *glyph = font_get_glyph(&font, 'B');

    context_init(&context, COMMAND_BUFFER_DEFAULT, NULL, 0, NULL);

    command_buffer_add_nop(&context.command_buffer);

    command_buffer_add_line(&context.command_buffer,
            (struct point){0, 0},
            (struct point){1, 1});

    command_buffer_add_rect(
            &context.command_buffer, (struct rect){{0, 0}, 1, 1});

    command_buffer_add_image(
            &context.command_buffer, (struct point){50, 2}, &glyph->image);

    while (true) {
        struct command command;
        if (command_buffer_pop(&context.command_buffer, &command)) {
            break;
        }

        stub_render_command(&context, &command);
    }

cleanup:
    context_free(&context);
    font_free(&font);
    return retval;
}
