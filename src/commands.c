#include "sunset/commands.h"
#include "cglm/mat4.h"

void command_nop_init(struct command *command) {
    command->type = COMMAND_NOP;
}

void command_line_init(
        struct command *command, struct point from, struct point to) {
    command->type = COMMAND_LINE;
    command->data.line = (struct command_line){from, to};
}

void command_rect_init(struct command *command, struct rect rect) {
    command->type = COMMAND_RECT;
    command->data.rect = (struct command_rect){rect};
}

void command_filled_rect_init(
        struct command *command, struct rect rect, struct color color) {
    command->type = COMMAND_FILLED_RECT;
    command->data.filled_rect = (struct command_filled_rect){rect, color};
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
    command->data.filled_arc = (struct command_filled_arc){center, r, a0, a1};
}

void command_text_init(struct command *command,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        enum window_point alignment) {
    command->type = COMMAND_TEXT;
    command->data.text =
            (struct command_text){start, font, text, text_len, alignment};
}

void command_image_init(
        struct command *command, struct point pos, struct image const *image) {
    command->type = COMMAND_IMAGE;
    command->data.image = (struct command_image){pos, *image};
}

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

void command_buffer_add_nop(struct command_buffer *command_buffer) {
    struct command command;
    command_nop_init(&command);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_line(struct command_buffer *command_buffer,
        struct point from,
        struct point to) {
    struct command command;
    command_line_init(&command, from, to);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_rect(
        struct command_buffer *command_buffer, struct rect rect) {
    struct command command;
    command_rect_init(&command, rect);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_filled_rect(struct command_buffer *command_buffer,
        struct rect rect,
        struct color color) {
    struct command command;
    command_filled_rect_init(&command, rect, color);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_arc(struct command_buffer *command_buffer,
        struct point center,
        int r,
        float a0,
        float a1) {
    struct command command;
    command_arc_init(&command, center, r, a0, a1);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_filled_arc(struct command_buffer *command_buffer,
        struct point center,
        uint32_t r,
        float a0,
        float a1) {
    struct command command;
    command_filled_arc_init(&command, center, r, a0, a1);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_text(struct command_buffer *command_buffer,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        enum window_point alignment) {
    struct command command;
    command_text_init(&command, start, font, text, text_len, alignment);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_image(struct command_buffer *command_buffer,
        struct point pos,
        struct image const *image) {
    struct command command;
    command_image_init(&command, pos, image);
    command_buffer_append(command_buffer, &command);
}

void command_buffer_add_custom_command(struct command_buffer *command_buffer,
        uint8_t command_type,
        uint8_t seq_num,
        void *data) {
    struct command command;
    command.type = command_type;
    command.seq_num = seq_num;
    memcpy(&command.data, data, sizeof(command.data));
    command_buffer_append(command_buffer, &command);
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

void command_buffer_add_mesh(struct command_buffer *command_buffer,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform) {
    struct command command;
    command_mesh_init(&command, mesh_id, texture_id, transform);
    command_buffer_append(command_buffer, &command);
}

bool command_buffer_empty(struct command_buffer *command_buffer) {
    return ring_buffer_empty(&command_buffer->ring_buffer);
}

void command_set_zindex_init(struct command *command, size_t z) {
    command->type = COMMAND_SET_ZINDEX;
    command->data.set_zindex = (struct command_set_zindex){z};
}

void command_buffer_add_set_zindex(
        struct command_buffer *command_buffer, size_t z) {
    struct command command;
    command_set_zindex_init(&command, z);
    command_buffer_append(command_buffer, &command);
}
