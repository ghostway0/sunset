#include <stddef.h>

#include "sunset/backend.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/vector.h"

#include "sunset/ui.h"

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
    MouseMoveEvent *mouse_move = (MouseMoveEvent *)event.data;
    Widget *current = context->active_ui->current_widget;

    if (!current) {
        current = context->active_ui->root;
    }

    context->active_ui->current_widget =
            find_active_widget(current, mouse_move->absolute);
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

    vector_init(context->ui_contexts);
}
