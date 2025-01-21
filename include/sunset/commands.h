#pragma once

#include <stdint.h>

#include <cglm/types.h>

#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/render.h"
#include "sunset/ring_buffer.h"

struct font;

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
    COMMAND_CUSTOM,
    NUM_COMMANDS,
};

struct command_nop {};

struct command_line {
    struct point from;
    struct point to;
};

struct command_rect {
    struct rect bounds;
    Color color;
};

struct command_filled_rect {
    struct rect rect;
    Color color;
};

struct command_arc {
    struct point center;
    uint32_t r;
    float a0;
    float a1;
};

struct command_filled_arc {
    struct point center;
    uint32_t r;
    float a0;
    float a1;
};

struct command_text {
    struct point start;
    struct font *font;
    char const *text;
    uint32_t text_len;
    WindowPoint origin;
};

struct command_image {
    struct point pos;
    struct image image;
};

struct command_mesh {
    bool instanced;
    bool textured;
    uint32_t mesh_id;
    uint32_t texture_id;
    mat4 transform;
};

struct command_set_zindex {
    size_t zindex;
};

struct command_custom {
    struct program *program;
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
        struct command_image image;
        struct command_mesh mesh;
        struct command_set_zindex set_zindex;
        struct command_custom custom;
    } data;
};

void command_nop_init(struct command *command);

void command_line_init(
        struct command *command, struct point from, struct point to);

void command_rect_init(
        struct command *command, struct rect rect, Color color);

void command_filled_rect_init(
        struct command *command, struct rect rect, Color color);

void command_arc_init(struct command *command,
        struct point center,
        int r,
        float a0,
        float a1);

void command_filled_arc_init(struct command *command,
        struct point center,
        uint32_t r,
        float a0,
        float a1);

void command_text_init(struct command *command,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        WindowPoint origin);

void command_image_init(struct command *command,
        struct point pos,
        struct image const *image);

void command_custom_init(struct command *command, struct program *program);

void command_mesh_init(struct command *command,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform);

void command_set_zindex_init(struct command *command, size_t zindex);

struct CommandBufferOptions {
    size_t buffer_size;
} typedef CommandBufferOptions;

#define COMMAND_BUFFER_DEFAULT                                             \
    (CommandBufferOptions) {                                               \
        .buffer_size = 1024                                                \
    }

struct CommandBuffer {
    RingBuffer ring_buffer;
} typedef CommandBuffer;

void cmdbuf_init(CommandBuffer *cmdbuf, CommandBufferOptions options);

void cmdbuf_destroy(CommandBuffer *cmdbuf);

void cmdbuf_append(CommandBuffer *cmdbuf, struct command const *command);

int cmdbuf_pop(CommandBuffer *cmdbuf, struct command *command_out);

void cmdbuf_clear(CommandBuffer *cmdbuf);

void cmdbuf_add_nop(CommandBuffer *cmdbuf);

void cmdbuf_add_line(
        CommandBuffer *cmdbuf, struct point from, struct point to);

void cmdbuf_add_rect(CommandBuffer *cmdbuf, struct rect rect, Color color);

void cmdbuf_add_filled_rect(
        CommandBuffer *cmdbuf, struct rect rect, Color color);

void cmdbuf_add_arc(CommandBuffer *cmdbuf,
        struct point center,
        size_t r,
        float a0,
        float a1);

void cmdbuf_add_filled_arc(CommandBuffer *cmdbuf,
        struct point center,
        size_t r,
        float a0,
        float a1);

void cmdbuf_add_text(CommandBuffer *cmdbuf,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len,
        WindowPoint origin);

void cmdbuf_add_image(
        CommandBuffer *cmdbuf, struct point pos, struct image const *image);

void cmdbuf_add_mesh(CommandBuffer *cmdbuf,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform);

void cmdbuf_add_set_zindex(CommandBuffer *cmdbuf, size_t zindex);

bool cmdbuf_empty(CommandBuffer *cmdbuf);
