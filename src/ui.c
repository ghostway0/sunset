#include <stddef.h>

#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/vector.h"

#include "sunset/ui.h"

static struct widget *find_active_widget(
        struct widget *current, struct point mouse) {
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

static void mouse_click_handler(
        struct engine_context *context, struct event event) {
    unused(event);

    struct widget *current = context->active_ui->current_widget;

    if (current->tag == WIDGET_BUTTON) {
        current->button.clicked_callback(context);
    }
}

static void key_up_handler(struct engine_context *context, struct event event) {
    struct widget *current = context->active_ui->current_widget;

    if (!current || current->tag != WIDGET_INPUT) {
        return;
    }

    if (!current->input.text) {
        vector_init(current->input.text);
    }

    if (event.key_up == '\b') {
        vector_pop_back(current->input.text);
    } else {
        vector_append(current->input.text, event.key_up);
    }
}

static void mouse_move_handler(
        struct engine_context *context, struct event event) {
    struct widget *current = context->active_ui->current_widget;

    if (!current) {
        current = context->active_ui->root;
    }

    context->active_ui->current_widget =
            find_active_widget(current, event.mouse_move.absolute);
}

void ui_setup(struct engine_context *context) {
    event_queue_add_handler(
            &context->event_queue, SYSTEM_EVENT_MOUSE_MOVE, mouse_move_handler);

    event_queue_add_handler(&context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            mouse_click_handler);

    event_queue_add_handler(
            &context->event_queue, SYSTEM_EVENT_KEY_UP, key_up_handler);
}
