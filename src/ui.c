#include <stddef.h>
#include <stdlib.h>

#include "internal/mem_utils.h"
#include "internal/utils.h"
#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/input.h"
#include "sunset/render.h"
#include "sunset/rman.h"
#include "sunset/vector.h"

#include "sunset/ui.h"

DECLARE_RESOURCE_ID(input_focus);

// NOTE: this could be done with entities and plugins

static Widget *find_active_widget(Widget *current, Point mouse) {
    // backtrack
    while (current && !point_within_rect(mouse, current->bounds)) {
        current = current->parent;
    }

    if (!current) {
        return NULL;
    }

    bool found = true;

    while (found) {
        if (!current->children) {
            break;
        }

        found = false;

        for (size_t i = 0; i < vector_size(current->children); i++) {
            if (point_within_rect(mouse, current->children[i]->bounds)) {
                current = current->children[i];
                found = true;
                break;
            }
        }
    }

    return current;
}

static void mouse_click_handler(EngineContext *context, void *, Event) {
    Focus *focus = rman_get(&context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_UI) {
        return;
    }

    Widget *current = context->active_ui->current_widget;

    if (!current) {
        return;
    }

    // NOTE: the correct calculation here would be to go up
    // the actual tree. subwidgets currently shadow buttons.
    // the current hack is to set to a size of 0,0.

    if (current->tag == WIDGET_BUTTON && current->button.clicked_callback) {
        current->button.clicked_callback(context);
    }
}

static void key_handler(EngineContext *context, void *, Event event) {
    if (!context->active_ui) {
        return;
    }

    Widget *current = context->active_ui->current_widget;

    if (!current || current->tag != WIDGET_INPUT) {
        return;
    }

    if (!current->input.text) {
        vector_init(current->input.text);
    }

    Key *key = (Key *)event.data;

    if (*key == KEY_BACKSPACE) {
        if (vector_size(current->input.text) > 0) {
            vector_pop_back(current->input.text);
        }
    } else {
        vector_append(current->input.text, *key);
    }
}

static void mouse_move_handler(
        EngineContext *context, void *, Event event) {
    uint32_t *focus = rman_get(&context->rman, RESOURCE_ID(input_focus));

    if (!context->active_ui || !one_matches(*focus, FOCUS_UI, FOCUS_NULL)) {
        return;
    }

    MouseMoveEvent *mouse_move = (MouseMoveEvent *)event.data;
    Widget *current = context->active_ui->current_widget;

    if (!current) {
        current = context->active_ui->root;
    }

    context->active_ui->current_widget =
            find_active_widget(current, mouse_move->absolute);

    *focus = context->active_ui->current_widget ? FOCUS_UI : FOCUS_NULL;
}

static void render_widget(CommandBuffer *cmdbuf, Widget const *widget) {
    if (!widget->active) {
        return;
    }

    // not sure why this draws ontop
    if (widget->children) {
        for (size_t i = 0; i < vector_size(widget->children); i++) {
            render_widget(cmdbuf, widget->children[i]);
        }
    }

    switch (widget->tag) {
        case WIDGET_CONTAINER:
        case WIDGET_BUTTON:
            if (!widget->style.solid) {
                cmdbuf_add_rect(cmdbuf,
                        widget->bounds,
                        widget->style.color,
                        WINDOW_TOP_LEFT);
            } else {
                cmdbuf_add_filled_rect(cmdbuf,
                        widget->bounds,
                        widget->style.color,
                        WINDOW_TOP_LEFT);
            }
            break;
        case WIDGET_TEXT:
            cmdbuf_add_text(cmdbuf,
                    rect_get_origin(widget->bounds),
                    widget->text.font,
                    widget->text.input,
                    strlen(widget->text.input),
                    widget->text.size,
                    WINDOW_TOP_LEFT);
            break;
        case WIDGET_INPUT:
            cmdbuf_add_rect(cmdbuf,
                    widget->bounds,
                    widget->style.color,
                    WINDOW_TOP_LEFT);

            if (!widget->input.text) {
                break;
            }

            cmdbuf_add_text(cmdbuf,
                    rect_get_origin(widget->bounds),
                    widget->input.font,
                    widget->input.text,
                    vector_size(widget->input.text),
                    widget->input.text_size,
                    WINDOW_TOP_LEFT);
            break;
        case WIDGET_IMAGE:
            // TODO: resizing?
            cmdbuf_add_image(cmdbuf,
                    (Point){.x = widget->bounds.x, .y = widget->bounds.y},
                    &widget->image);
            break;
        default:
            todo();
    }
}

static void tick_handler(EngineContext *engine_context,
        void * /*local_context*/,
        Event /*event*/) {
    if (!engine_context->active_ui) {
        return;
    }

    render_widget(&engine_context->cmdbuf, engine_context->active_ui->root);
}

void ui_init(UIContext *ui_out) {
    ui_out->root = sunset_malloc(sizeof(struct Widget));
    ui_out->current_widget = NULL;

    *ui_out->root = (Widget){
            .children = NULL,
            .active = true,
            .bounds = {0},
            .tag = WIDGET_CONTAINER,
            .style = {},
    };
}

static int ui_add_widget_impl(Widget *root, Widget *widget) {
    if (!root->children) {
        vector_init(root->children);
    }

    if (rect_contains(root->bounds, widget->bounds)) {
        for (size_t i = 0; i < vector_size(root->children); i++) {
            Widget *child = root->children[i];

            if (rect_contains(child->bounds, widget->bounds)) {
                return ui_add_widget(child, widget);
            }
        }

        widget->parent = root;

        vector_append(root->children, widget);

        return 0;
    }

    // XXX: maybe drop the requirement for this setup
    // and just do it here.
    if (root->tag == WIDGET_CONTAINER) {
        if (is_zero_rect(root->bounds)) {
            root->bounds = widget->bounds;
        } else {
            root->bounds = rect_closure(root->bounds, widget->bounds);
        }

        widget->parent = root;

        vector_append(root->children, widget);

        return 0;
    }

    return -1;
}

/// Takes ownership over `widget`
int ui_add_widget(Widget *root, Widget *widget) {
    if (widget->style.relative) {
        widget->bounds.x += root->bounds.x;
        widget->bounds.y += root->bounds.y;
    }

    return ui_add_widget_impl(root, widget);
}

static void destroy_widget(Widget *widget) {
    for (size_t i = 0; i < vector_size(widget->children); i++) {
        destroy_widget(widget->children[i]);
    }

    free(widget);
}

void ui_destroy(UIContext *ui) {
    destroy_widget(ui->root);
}

void ui_setup(EngineContext *context) {
    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_MOUSE_MOVE,
            (EventHandler){context, mouse_move_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){context, mouse_click_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_KEY_PRESSED,
            (EventHandler){context, key_handler});
    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_KEY_REPEAT,
            (EventHandler){context, key_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_TICK,
            (EventHandler){NULL, tick_handler});

    vector_init(context->ui_contexts);
}
