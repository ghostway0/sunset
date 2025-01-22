#include <stddef.h>

#include "commands.h"
#include "internal/utils.h"
#include "log.h"
#include "render.h"
#include "sunset/backend.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/vector.h"

#include "sunset/ui.h"

// NOTE: this could be done with entities and plugins

static Widget *find_active_widget(Widget *current, struct point mouse) {
    // backtrack
    while (!point_within_rect(mouse, current->bounds)) {
        current = current->parent;
    }

    if (!current) {
        return NULL;
    }

    while (point_within_rect(mouse, current->bounds)) {
        if (!current->children) {
            return current;
        }

        for (size_t i = 0; i < vector_size(current->children); i++) {
            if (point_within_rect(mouse, current->children[i]->bounds)) {
                current = current->children[i];
                break;
            }
        }
    }

    return current->parent;
}

static void mouse_click_handler(EngineContext *context, void *, Event) {
    Widget *current = context->active_ui->current_widget;

    if (current->tag == WIDGET_BUTTON) {
        current->button.clicked_callback(context);
    }
}

static void key_up_handler(EngineContext *context, void *, Event event) {
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

    Key *key_up = (Key *)event.data;

    if (*key_up == KEY_BACKSPACE) {
        vector_pop_back(current->input.text);
    } else {
        vector_append(current->input.text, *key_up);
    }
}

static void mouse_move_handler(
        EngineContext *context, void *, Event event) {
    if (!context->active_ui) {
        return;
    }

    MouseMoveEvent *mouse_move = (MouseMoveEvent *)event.data;
    Widget *current = context->active_ui->current_widget;

    if (!current) {
        current = context->active_ui->root;
    }

    context->active_ui->current_widget =
            find_active_widget(current, mouse_move->absolute);
}

static void render_widget(CommandBuffer *cmdbuf, Widget const *widget) {
    switch (widget->tag) {
        case WIDGET_BUTTON: {
            if (widget->style.background_transparent) {
                cmdbuf_add_rect(
                        cmdbuf, widget->bounds, widget->style.color);
            } else {
                cmdbuf_add_filled_rect(
                        cmdbuf, widget->bounds, widget->style.color);
            }
            break;
        }
        case WIDGET_TEXT:
            cmdbuf_add_text(cmdbuf,
                    rect_get_origin(widget->bounds),
                    widget->text.font,
                    widget->text.input,
                    vector_size(widget->text.input),
                    WINDOW_TOP_LEFT);
            break;
        case WIDGET_INPUT:
            cmdbuf_add_rect(cmdbuf, widget->bounds, widget->style.color);
            cmdbuf_add_text(cmdbuf,
                    rect_get_origin(widget->bounds),
                    widget->input.font,
                    widget->input.text,
                    vector_size(widget->input.text),
                    WINDOW_TOP_LEFT);
            break;
        case WIDGET_IMAGE:
            // TODO: resizing?
            cmdbuf_add_image(cmdbuf,
                    (struct point){
                            .x = widget->bounds.x, .y = widget->bounds.y},
                    &widget->image);
            break;
        case WIDGET_CONTAINER:
            // do nothing
            break;
        default:
            todo();
    }

    if (widget->children) {
        for (size_t i = 0; i < vector_size(widget->children); i++) {
            render_widget(cmdbuf, widget->children[i]);
        }
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

void ui_setup(EngineContext *context) {
    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_MOUSE_MOVE,
            (EventHandler){context, mouse_move_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){context, mouse_click_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_KEY_UP,
            (EventHandler){context, key_up_handler});

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_TICK,
            (EventHandler){NULL, tick_handler});
    ;
    vector_init(context->ui_contexts);
}
