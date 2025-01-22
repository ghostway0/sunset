#include <string.h>

#include <cglm/mat4.h>

#include "render.h"
#include "sunset/commands.h"

void command_nop_init(struct command *command) {
    command->type = COMMAND_NOP;
}

void command_line_init(
        struct command *command, struct point from, struct point to) {
    command->type = COMMAND_LINE;
    command->data.line = (struct command_line){from, to};
}

void command_rect_init(struct command *command,
        struct rect rect,
        Color color,
        WindowPoint origin) {
    command->type = COMMAND_RECT;
    command->data.rect = (struct command_rect){origin, rect, color};
}

void command_filled_rect_init(struct command *command,
        struct rect rect,
        Color color,
        WindowPoint origin) {
    command->type = COMMAND_FILLED_RECT;
    command->data.filled_rect =
            (struct command_filled_rect){origin, rect, color};
}

void command_arc_init(struct command *command,
        struct point center,
        int r,
        float a0,
        float a1) {
    command->type = COMMAND_ARC;
    command->data.arc = (struct command_arc){center, r, a0, a1};
}

void command_filled_arc_init(struct command *command,
        struct point center,
        uint32_t r,
        float a0,
        float a1) {
    command->type = COMMAND_FILLED_ARC;
    command->data.filled_arc =
            (struct command_filled_arc){center, r, a0, a1};
}

void command_text_init(struct command *command,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        WindowPoint origin) {
    command->type = COMMAND_TEXT;
    command->data.text =
            (struct command_text){start, font, text, text_len, origin};
}

void command_image_init(struct command *command,
        struct point pos,
        struct image const *image) {
    command->type = COMMAND_IMAGE;
    command->data.image = (struct command_image){pos, *image};
}

void cmdbuf_init(CommandBuffer *cmdbuf, CommandBufferOptions options) {
    ring_buffer_init(&cmdbuf->ring_buffer,
            options.buffer_size,
            sizeof(struct command));
}

void cmdbuf_destroy(CommandBuffer *cmdbuf) {
    ring_buffer_destroy(&cmdbuf->ring_buffer);
}

void cmdbuf_append(CommandBuffer *cmdbuf, struct command const *command) {
    ring_buffer_append(&cmdbuf->ring_buffer, command);
}

int cmdbuf_pop(CommandBuffer *cmdbuf, struct command *command_out) {
    return ring_buffer_pop(&cmdbuf->ring_buffer, command_out);
}

void cmdbuf_clear(CommandBuffer *cmdbuf) {
    ring_buffer_clear(&cmdbuf->ring_buffer);
}

void cmdbuf_add_nop(CommandBuffer *cmdbuf) {
    struct command command;
    command_nop_init(&command);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_line(
        CommandBuffer *cmdbuf, struct point from, struct point to) {
    struct command command;
    command_line_init(&command, from, to);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_rect(CommandBuffer *cmdbuf,
        struct rect rect,
        Color color,
        WindowPoint origin) {
    struct command command;
    command_rect_init(&command, rect, color, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_filled_rect(CommandBuffer *cmdbuf,
        struct rect rect,
        Color color,
        WindowPoint origin) {
    struct command command;
    command_filled_rect_init(&command, rect, color, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_arc(CommandBuffer *cmdbuf,
        struct point center,
        size_t r,
        float a0,
        float a1) {
    struct command command;
    command_arc_init(&command, center, r, a0, a1);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_filled_arc(CommandBuffer *cmdbuf,
        struct point center,
        size_t r,
        float a0,
        float a1) {
    struct command command;
    command_filled_arc_init(&command, center, r, a0, a1);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_text(CommandBuffer *cmdbuf,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        WindowPoint origin) {
    struct command command;
    command_text_init(&command, start, font, text, text_len, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_image(CommandBuffer *cmdbuf,
        struct point pos,
        struct image const *image) {
    struct command command;
    command_image_init(&command, pos, image);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_custom_command(CommandBuffer *cmdbuf,
        uint8_t command_type,
        uint8_t seq_num,
        void *data) {
    struct command command;
    command.type = command_type;
    command.seq_num = seq_num;
    memcpy(&command.data, data, sizeof(command.data));
    cmdbuf_append(cmdbuf, &command);
}

// TODO: add arguments
void command_custom_init(struct command *command, struct program *program) {
    command->type = COMMAND_CUSTOM;
    command->data.custom.program = program;
}

void command_mesh_init(struct command *command,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform) {
    command->type = COMMAND_MESH;
    command->data.mesh = (struct command_mesh){
            true,
            false,
            mesh_id,
            texture_id,
            GLM_MAT4_IDENTITY_INIT,
    };

    glm_mat4_copy(transform, command->data.mesh.transform);
}

void cmdbuf_add_mesh(CommandBuffer *cmdbuf,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform) {
    struct command command;
    command_mesh_init(&command, mesh_id, texture_id, transform);
    cmdbuf_append(cmdbuf, &command);
}

bool cmdbuf_empty(CommandBuffer *cmdbuf) {
    return ring_buffer_empty(&cmdbuf->ring_buffer);
}

void command_set_zindex_init(struct command *command, size_t z) {
    command->type = COMMAND_SET_ZINDEX;
    command->data.set_zindex = (struct command_set_zindex){z};
}

void cmdbuf_add_set_zindex(CommandBuffer *cmdbuf, size_t z) {
    struct command command;
    command_set_zindex_init(&command, z);
    cmdbuf_append(cmdbuf, &command);
}
