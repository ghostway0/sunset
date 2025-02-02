#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/vector.h"

typedef struct EngineContext EngineContext;
typedef struct Font Font;

typedef struct Style {
    bool solid;
    Color color;
    // relative-to-parent positioning
    bool relative;
} Style;

typedef struct Widget {
    Style style;
    vector(struct Widget *) children;
    struct Widget *parent;
    Rect bounds;
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
        struct {
            char const *input;
            Font *font;
            size_t size;
        } text;
        Image image;
        struct {
            vector(char) text;
            Font *font;
            size_t text_size;
        } input;
        struct {
            void (*clicked_callback)(EngineContext *);
        } button;
    };
} Widget;

typedef struct UIContext {
    Widget *root;
    Widget *current_widget;
} UIContext;

void ui_setup(EngineContext *context);

void ui_init(UIContext *ui_out);

int ui_add_widget(Widget *root, Widget *widget);
