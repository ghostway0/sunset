#include <string.h>

#include <cglm/mat4.h>

#include "sunset/commands.h"
#include "sunset/render.h"

void command_nop_init(Command *command) {
    command->type = COMMAND_NOP;
}

void command_line_init(Command *command, Point from, Point to) {
    command->type = COMMAND_LINE;
    command->line = (CommandLine){from, to};
}

void command_rect_init(
        Command *command, Rect rect, Color color, WindowPoint origin) {
    command->type = COMMAND_RECT;
    command->rect = (CommandRect){origin, rect, color};
}

void command_filled_rect_init(
        Command *command, Rect rect, Color color, WindowPoint origin) {
    command->type = COMMAND_FILLED_RECT;
    command->filled_rect = (CommandFilledRect){origin, rect, color};
}

void command_arc_init(
        Command *command, Point center, int r, float a0, float a1) {
    command->type = COMMAND_ARC;
    command->arc = (CommandArc){center, r, a0, a1};
}

void command_filled_arc_init(
        Command *command, Point center, uint32_t r, float a0, float a1) {
    command->type = COMMAND_FILLED_ARC;
    command->filled_arc = (struct CommandFilledArc){center, r, a0, a1};
}

void command_text_init(Command *command,
        Point start,
        Font *font,
        char const *text,
        uint32_t text_len,
        size_t size,
        WindowPoint origin) {
    command->type = COMMAND_TEXT;
    command->text =
            (struct CommandText){start, font, text, text_len, size, origin};
}

void command_image_init(
        Command *command, Point pos, Rect bounds, Image const *image) {
    command->type = COMMAND_IMAGE;
    command->image = (CommandImage){bounds, pos, *image};
}

void cmdbuf_init(CommandBuffer *cmdbuf, CommandBufferOptions options) {
    ring_buffer_init(
            &cmdbuf->ring_buffer, options.buffer_size, sizeof(Command));
}

void cmdbuf_destroy(CommandBuffer *cmdbuf) {
    ring_buffer_destroy(&cmdbuf->ring_buffer);
}

void cmdbuf_append(CommandBuffer *cmdbuf, Command const *command) {
    ring_buffer_append(&cmdbuf->ring_buffer, command);
}

int cmdbuf_pop(CommandBuffer *cmdbuf, Command *command_out) {
    return ring_buffer_pop(&cmdbuf->ring_buffer, command_out);
}

void cmdbuf_clear(CommandBuffer *cmdbuf) {
    ring_buffer_clear(&cmdbuf->ring_buffer);
}

void cmdbuf_add_nop(CommandBuffer *cmdbuf) {
    Command command;
    command_nop_init(&command);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_line(CommandBuffer *cmdbuf, Point from, Point to) {
    Command command;
    command_line_init(&command, from, to);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_rect(
        CommandBuffer *cmdbuf, Rect rect, Color color, WindowPoint origin) {
    Command command;
    command_rect_init(&command, rect, color, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_filled_rect(
        CommandBuffer *cmdbuf, Rect rect, Color color, WindowPoint origin) {
    Command command;
    command_filled_rect_init(&command, rect, color, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_arc(
        CommandBuffer *cmdbuf, Point center, size_t r, float a0, float a1) {
    Command command;
    command_arc_init(&command, center, r, a0, a1);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_filled_arc(
        CommandBuffer *cmdbuf, Point center, size_t r, float a0, float a1) {
    Command command;
    command_filled_arc_init(&command, center, r, a0, a1);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_text(CommandBuffer *cmdbuf,
        Point start,
        Font *font,
        char const *text,
        uint32_t text_len,
        size_t size,
        WindowPoint origin) {
    Command command;
    command_text_init(&command, start, font, text, text_len, size, origin);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_image(
        CommandBuffer *cmdbuf, Point pos, Rect bounds, Image const *image) {
    Command command;
    command_image_init(&command, pos, bounds, image);
    cmdbuf_append(cmdbuf, &command);
}

void command_mesh_init(
        Command *command, uint32_t mesh_id, uint32_t texture_id) {
    command->type = COMMAND_MESH;
    command->mesh = (CommandMesh){
            .instanced = true,
            .mesh_id = mesh_id,
            .texture_id = texture_id,
    };
}

void cmdbuf_add_mesh(
        CommandBuffer *cmdbuf, uint32_t mesh_id, uint32_t texture_id) {
    Command command;
    command_mesh_init(&command, mesh_id, texture_id);
    cmdbuf_append(cmdbuf, &command);
}

bool cmdbuf_empty(CommandBuffer *cmdbuf) {
    return ring_buffer_empty(&cmdbuf->ring_buffer);
}

void command_set_zindex_init(Command *command, size_t z) {
    command->type = COMMAND_SET_ZINDEX;
    command->set_zindex = (CommandSetZIndex){z};
}

void cmdbuf_add_set_zindex(CommandBuffer *cmdbuf, size_t z) {
    Command command;
    command_set_zindex_init(&command, z);
    cmdbuf_append(cmdbuf, &command);
}

void cmdbuf_add_multiple(CommandBuffer *cmdbuf,
        Command const *commands,
        size_t count,
        EntityRenderContext *context) {
    Command csc = {.type = COMMAND_SET_CONTEXT, .set_context = {context}};
    cmdbuf_append(cmdbuf, &csc);

    for (size_t i = 0; i < count; i++) {
        cmdbuf_append(cmdbuf, &commands[i]);
    }
}
