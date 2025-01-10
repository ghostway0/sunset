#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/vector.h"

typedef struct EngineContext EngineContext;
struct input_state;

typedef struct Style {
    // STUB
} Style;

typedef struct Widget {
    Style style;
    vector(struct Widget *) children;
    struct Widget *parent;
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
            void (*clicked_callback)(EngineContext *);
        } button;
    };
} Widget;

typedef struct UIContext {
    vector(Widget) widgets;
    Widget *root;
    Widget *current_widget;
} UIContext;

void ui_setup(EngineContext *context);
