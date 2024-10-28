#pragma once

#include <stdint.h>
#include <string.h>

#include "cglm/types.h"
#include "sunset/color.h"
#include "sunset/geometry.h"
#include "sunset/ring_buffer.h"

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
    COMMAND_CUSTOM,
    NUM_COMMANDS,
};

struct command_nop {};

struct command_line {
    struct point from;
    struct point to;
};

struct command_rect {
    struct rect rect;
};

struct command_filled_rect {
    struct rect rect;
    struct color color;
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
        struct command_custom custom;
    } data;
};

void command_nop_init(struct command *command);

void command_line_init(
        struct command *command, struct point from, struct point to);

void command_rect_init(struct command *command, struct rect rect);

void command_filled_rect_init(
        struct command *command, struct rect rect, struct color color);

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
        uint32_t text_len);

void command_image_init(
        struct command *command, struct point pos, struct image const *image);

void command_custom_init(struct command *command, struct program *program);

void command_mesh_init(struct command *command,
        bool instanced,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform);

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
        struct command_buffer_options options);

void command_buffer_free(struct command_buffer *command_buffer);

void command_buffer_append(
        struct command_buffer *command_buffer, struct command const *command);

int command_buffer_pop(
        struct command_buffer *command_buffer, struct command *command_out);

void command_buffer_clear(struct command_buffer *command_buffer);

void command_buffer_add_nop(struct command_buffer *command_buffer);

void command_buffer_add_line(struct command_buffer *command_buffer,
        struct point from,
        struct point to);

void command_buffer_add_rect(
        struct command_buffer *command_buffer, struct rect rect);

void command_buffer_add_filled_rect(struct command_buffer *command_buffer,
        struct rect rect,
        struct color color);

void command_buffer_add_arc(struct command_buffer *command_buffer,
        struct point center,
        int r,
        float a0,
        float a1);

void command_buffer_add_filled_arc(struct command_buffer *command_buffer,
        struct point center,
        uint32_t r,
        float a0,
        float a1);

void command_buffer_add_text(struct command_buffer *command_buffer,
        struct point start,
        struct font *font,
        char const *text,
        uint32_t text_len);

void command_buffer_add_image(struct command_buffer *command_buffer,
        struct point pos,
        struct image const *image);

void command_buffer_add_mesh(struct command_buffer *command_buffer,
        bool instanced,
        uint32_t mesh_id,
        uint32_t texture_id,
        mat4 transform);

bool command_buffer_empty(struct command_buffer *command_buffer);
