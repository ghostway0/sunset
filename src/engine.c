#include "sunset/render.h"

// engine tick:
// 1. send SYSTEM_EVENT_TICK event
// 2. active ui elements (buttons, text boxes etc.)
// 3. physics for enabled objects (here we can use the 'zones' structure to have
// an interaction zone, no-physics zone etc. which would be managed by the
// engine)
// 4. ?????
// 5. profit
// should this be done _every_ frame?

void mouse_ui_handler(struct context *context, struct event event) {
    struct point mouse_move = event.data.mouse_move;

    struct ui_context *active_ui = context->active_ui;

    for (size_t i = 0; i < active_ui->num_buttons; i++) {
        struct button button = active_ui->buttons[i];

        if (point_within_rect(mouse_move, button.bounds)) {
            button.clicked_callback(context);
        }
    }
}

void engine_setup(struct context *context) {
    event_queue_add_handler(
            context->event_queue, SYSTEM_EVENT_MOUSE, mouse_ui_handler);
}

int engine_tick(struct context *context) {
    // 1. send tick event
    event_queue_process_one(context,
            context->event_queue,
            (struct event){.type_id = SYSTEM_EVENT_TICK});

    // 2. play ui elements

    return 0;
}
