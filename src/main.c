#include <stddef.h>
#include <string.h>

#include <log.h>

#include "sunset/fonts.h"
#include "sunset/ring_buffer.h"

enum command_type : uint8_t {
    COMMAND_NOP,
    COMMAND_LINE,
    COMMAND_RECT,
    COMMAND_FILLED_RECT,
    COMMAND_ARC,
    COMMAND_FILLED_ARC,
    COMMAND_TEXT,
    NUM_COMMANDS,
};

#define MAX_NUM_CUSTOM_COMMANDS (256 - NUM_COMMANDS)

struct command_nop {};

struct command_line {
    int x0;
    int y0;
    int x1;
    int y1;
};

struct command_rect {
    int x;
    int y;
    int w;
    int h;
};

struct command_filled_rect {
    int x;
    int y;
    int w;
    int h;
};

struct command_arc {
    int cx;
    int cy;
    int r;
    float a0;
    float a1;
};

struct command_filled_arc {
    int cx;
    int cy;
    int r;
    float a0;
    float a1;
};

struct command_text {
    int x;
    int y;
    struct font *font;
    char const *text;
    uint32_t text_len;
};

struct command {
    enum command_type type;
    uint8_t seq_num;

    union {
        struct command_nop nop;
        struct command_line line;
        struct command_rect rect;
        struct command_filled_rect filled_rect;
        struct command_arc arc;
        struct command_filled_arc filled_arc;
        struct command_text text;
    } data;
};

void command_nop_init(struct command *command) {
    command->type = COMMAND_NOP;
}

void command_line_init(
        struct command *command, int x0, int y0, int x1, int y1) {
    command->type = COMMAND_LINE;
    command->data.line = (struct command_line){x0, y0, x1, y1};
}

void command_rect_init(struct command *command, int x, int y, int w, int h) {
    command->type = COMMAND_RECT;
    command->data.rect = (struct command_rect){x, y, w, h};
}

void command_filled_rect_init(
        struct command *command, int x, int y, int w, int h) {
    command->type = COMMAND_FILLED_RECT;
    command->data.filled_rect = (struct command_filled_rect){x, y, w, h};
}

void command_arc_init(
        struct command *command, int cx, int cy, int r, float a0, float a1) {
    command->type = COMMAND_ARC;
    command->data.arc = (struct command_arc){cx, cy, r, a0, a1};
}

void command_filled_arc_init(
        struct command *command, int cx, int cy, int r, float a0, float a1) {
    command->type = COMMAND_FILLED_ARC;
    command->data.filled_arc = (struct command_filled_arc){cx, cy, r, a0, a1};
}

void command_text_init(struct command *command,
        int x,
        int y,
        struct font *font,
        char const *text,
        uint32_t text_len) {
    command->type = COMMAND_TEXT;
    command->data.text = (struct command_text){x, y, font, text, text_len};
}

struct command_buffer_options {
    size_t buffer_size;
};

#define COMMAND_BUFFER_DEFAULT                                                 \
    (struct command_buffer_options) {                                          \
        .buffer_size = 1024                                                    \
    }

struct command_buffer {
    struct ring_buffer ring_buffer;
};

void command_buffer_init(struct command_buffer *command_buffer,
        struct command_buffer_options options) {
    ring_buffer_init(&command_buffer->ring_buffer,
            options.buffer_size,
            sizeof(struct command));
}

void command_buffer_free(struct command_buffer *command_buffer) {
    ring_buffer_free(&command_buffer->ring_buffer);
}

void command_buffer_append(
        struct command_buffer *command_buffer, struct command const *command) {
    ring_buffer_append(&command_buffer->ring_buffer, command);
}

int command_buffer_pop(
        struct command_buffer *command_buffer, struct command *command_out) {
    return ring_buffer_pop(&command_buffer->ring_buffer, command_out);
}

void command_buffer_clear(struct command_buffer *command_buffer) {
    ring_buffer_clear(&command_buffer->ring_buffer);
}

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

int stub_render_command(
        struct context *context, struct command const *command) {
    switch (command->type) {
        case COMMAND_NOP:
            log_info("command_nop");
            break;
        case COMMAND_LINE: {
            struct command_line const *line = &command->data.line;
            log_info("command_line: %d %d %d %d",
                    line->x0,
                    line->y0,
                    line->x1,
                    line->y1);
            break;
        }
        case COMMAND_RECT:
            log_info("command_rect %d %d %d %d",
                    command->data.rect.x,
                    command->data.rect.y,
                    command->data.rect.w,
                    command->data.rect.h);
            break;
        case COMMAND_FILLED_RECT:
            log_info("command_filled_rect %d %d %d %d",
                    command->data.filled_rect.x,
                    command->data.filled_rect.y,
                    command->data.filled_rect.w,
                    command->data.filled_rect.h);
            break;
        case COMMAND_ARC:
            log_info("command_arc %d %d %d %f %f",
                    command->data.arc.cx,
                    command->data.arc.cy,
                    command->data.arc.r,
                    command->data.arc.a0,
                    command->data.arc.a1);
            break;
        case COMMAND_FILLED_ARC:
            log_info("command_filled_arc %d %d %d %f %f",
                    command->data.filled_arc.cx,
                    command->data.filled_arc.cy,
                    command->data.filled_arc.r,
                    command->data.filled_arc.a0,
                    command->data.filled_arc.a1);
            break;
        case COMMAND_TEXT:
            log_info("command_text %d %d %s",
                    command->data.text.x,
                    command->data.text.y,
                    command->data.text.text);
            break;
        default: {
            size_t custom_command_idx = command->type - NUM_COMMANDS;
            // not sure I like this security wise
            if (context->custom_commands[custom_command_idx] != NULL) {
                context->custom_commands[custom_command_idx](
                        context->render_context, command);
            }
        }
    }

    return 0;
}

int main() {
    struct command_buffer command_buffer;
    struct context context;
    struct command commands[3];
    int retval = 0;

    memset(&context, 0, sizeof(context));

    command_buffer_init(&command_buffer, COMMAND_BUFFER_DEFAULT);

    command_nop_init(&commands[0]);
    command_line_init(&commands[1], 0, 0, 0, 1);
    command_arc_init(&commands[2], 0, 0, 1, 0.5f, 1.0f);

    for (size_t i = 0; i < sizeof(commands) / sizeof(struct command); i++) {
        command_buffer_append(&command_buffer, &commands[i]);
    }

    while (true) {
        struct command command;
        if (command_buffer_pop(&command_buffer, &command)) {
            break;
        }

        stub_render_command(&context, &command);
    }

    command_buffer_free(&command_buffer);

    struct font font;
    if ((retval = load_font_psf2("Inconsolata-32r.psf", "test", &font))) {
        log_error("load_font_psf2 failed: %d", retval);
        return retval;
    }

    struct glyph const *glyph = font_get_glyph(&font, 13);

    if (glyph == NULL) {
        log_error("font_get_glyph failed");
        return -1;
    }

    show_image_grayscale(&glyph->image);

    return retval;
}
