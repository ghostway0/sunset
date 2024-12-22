#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/vector.h"

struct engine_context;
struct input_state;

struct style {
    // STUB
};

struct widget {
    struct style style;
    vector(struct widget *) children;
    struct widget *parent;
    struct rect bounds;
    /// if inactive, all children are disabled
    bool active;

    enum {
        WIDGET_CONTAINER,
        WIDGET_TEXT,
        WIDGET_INPUT,
        WIDGET_IMAGE,
        WIDGET_BUTTON,
    } tag;

    union {
        char const *text;
        struct image image;
        struct {
            vector(char) text;
        } input;
        struct {
            void (*clicked_callback)(struct engine_context *);
        } button;
    };
};

struct ui_context {
    vector(struct widget) widgets;
    struct widget *root;
    struct widget *current_widget;
};

void ui_setup(struct engine_context *context);
