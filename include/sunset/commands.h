#pragma once

#include <stdint.h>

#include <cglm/types.h>

#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/render.h"
#include "sunset/ring_buffer.h"

typedef struct Font Font;

enum command_type : uint8_t {
    COMMAND_NOP,
    COMMAND_LINE,
    COMMAND_RECT,
    COMMAND_FILLED_RECT,
    COMMAND_ARC,
    COMMAND_FILLED_ARC,
    COMMAND_TEXT,
    COMMAND_IMAGE,
    COMMAND_MESH,
    COMMAND_SET_ZINDEX,
    COMMAND_SET_CONTEXT,
    NUM_COMMANDS,
};

typedef struct CommandNop {
} CommandNop;

typedef struct CommandLine {
    Point from;
    Point to;
} CommandLine;

typedef struct CommandRect {
    WindowPoint origin;
    Rect bounds;
    Color color;
} CommandRect;

typedef struct CommandFilledRect {
    WindowPoint origin;
    Rect rect;
    Color color;
} CommandFilledRect;

typedef struct CommandArc {
    Point center;
    uint32_t r;
    float a0;
    float a1;
} CommandArc;

typedef struct CommandFilledArc {
    Point center;
    uint32_t r;
    float a0;
    float a1;
} CommandFilledArc;

typedef struct CommandText {
    Point start;
    Font *font;
    char const *text;
    uint32_t text_len;
    size_t size;
    WindowPoint origin;
} CommandText;

typedef struct CommandImage {
    Rect bounds;
    Point pos;
    Image image;
} CommandImage;

typedef struct CommandMesh {
    bool instanced;
    uint32_t mesh_id;
    uint32_t texture_id;
} CommandMesh;

typedef struct CommandSetZIndex {
    size_t zindex;
} CommandSetZIndex;

typedef struct CommandSetContext {
    EntityRenderContext *context;
} CommandSetContext;

typedef struct Command {
    enum command_type type;
    uint8_t seq_num;

    union {
        CommandNop nop;
        CommandLine line;
        CommandRect rect;
        CommandFilledRect filled_rect;
        CommandArc arc;
        CommandFilledArc filled_arc;
        CommandText text;
        CommandImage image;
        CommandMesh mesh;
        CommandSetZIndex set_zindex;
        CommandSetContext set_context;
    };
} Command;

void command_nop_init(Command *command);

void command_line_init(Command *command, Point from, Point to);

void command_rect_init(
        Command *command, Rect rect, Color color, WindowPoint origin);

void command_filled_rect_init(
        Command *command, Rect rect, Color color, WindowPoint origin);

void command_arc_init(
        Command *command, Point center, int r, float a0, float a1);

void command_filled_arc_init(
        Command *command, Point center, uint32_t r, float a0, float a1);

void command_text_init(Command *command,
        Point start,
        Font *font,
        char const *text,
        uint32_t text_len,
        size_t size,
        WindowPoint origin);

void command_image_init(
        Command *command, Point pos, Rect bounds, Image const *image);

void command_mesh_init(
        Command *command, uint32_t mesh_id, uint32_t texture_id);

void command_set_zindex_init(Command *command, size_t zindex);

typedef struct CommandBufferOptions {
    size_t buffer_size;
} CommandBufferOptions;

#define COMMAND_BUFFER_DEFAULT (CommandBufferOptions){.buffer_size = 1024}

typedef struct CommandBuffer {
    RingBuffer ring_buffer;
} CommandBuffer;

void cmdbuf_init(CommandBuffer *cmdbuf, CommandBufferOptions options);

void cmdbuf_destroy(CommandBuffer *cmdbuf);

void cmdbuf_append(CommandBuffer *cmdbuf, Command const *command);

int cmdbuf_pop(CommandBuffer *cmdbuf, Command *command_out);

void cmdbuf_clear(CommandBuffer *cmdbuf);

void cmdbuf_add_nop(CommandBuffer *cmdbuf);

void cmdbuf_add_line(CommandBuffer *cmdbuf, Point from, Point to);

void cmdbuf_add_rect(
        CommandBuffer *cmdbuf, Rect rect, Color color, WindowPoint origin);

void cmdbuf_add_filled_rect(
        CommandBuffer *cmdbuf, Rect rect, Color color, WindowPoint origin);

void cmdbuf_add_arc(
        CommandBuffer *cmdbuf, Point center, size_t r, float a0, float a1);

void cmdbuf_add_filled_arc(
        CommandBuffer *cmdbuf, Point center, size_t r, float a0, float a1);

void cmdbuf_add_text(CommandBuffer *cmdbuf,
        Point start,
        Font *font,
        char const *text,
        uint32_t text_len,
        size_t size,
        WindowPoint origin);

void cmdbuf_add_image(
        CommandBuffer *cmdbuf, Point pos, Rect bounds, Image const *image);

void cmdbuf_add_mesh(
        CommandBuffer *cmdbuf, uint32_t mesh_id, uint32_t texture_id);

void cmdbuf_add_set_zindex(CommandBuffer *cmdbuf, size_t zindex);

bool cmdbuf_empty(CommandBuffer *cmdbuf);

void cmdbuf_add_multiple(CommandBuffer *cmdbuf,
        Command const *commands,
        size_t count,
        EntityRenderContext *context);
