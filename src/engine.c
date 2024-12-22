#include <stdint.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/physics.h"
#include "sunset/scene.h"
#include "sunset/ui.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

static constexpr float FRAME_TIME_S = 1.0f / 60.0f;

// engine tick:
// 1. send SYSTEM_EVENT_TICK event
// 2. active ui elements (buttons, text boxes etc.)
// 3. physics for enabled objects (here we can use the 'zones' structure to have
// an interaction zone, no-physics zone etc. which would be managed by the
// engine)
// 4. ?????
// 5. profit
// should this be done _every_ frame?

void engine_setup(struct engine_context *context) {
    // setup ui
    event_queue_add_handler(
            &context->event_queue, SYSTEM_EVENT_MOUSE_MOVE, mouse_ui_handler);
}

int engine_tick(struct engine_context *context) {
    // 1. send tick event
    event_queue_process_one(context,
            &context->event_queue,
            (struct event){.type_id = SYSTEM_EVENT_TICK});

    // 2. play ui elements
    // for (size_t i = 0; i < context->active_ui->num_buttons; i++) {
    //     struct button button = context->active_ui->buttons[i];
    //
    //     if ()
    //     button.bounds
    // }

    // 3. physics
    physics_step(&context->physics,
            context->scene,
            &context->event_queue,
            FRAME_TIME_S);

    // 4. process all generated events
    event_queue_process(context, &context->event_queue);

    return 0;
}

// TODO: get config
int engine_run(void) {
    int retval = 0;

    struct engine_context context;
    // TODO: setup

    if ((retval = backend_setup(
                 context.render_context, (struct render_config){}))) {
        goto cleanup;
    }

    while (!backend_should_stop(context.render_context)) {
        struct timespec timespec = get_time();

        // TODO: handle input using backend
        // backend_generate_input_events(context.render_context);

        if ((retval = engine_tick(&context))) {
            goto cleanup;
        }

        if ((retval = scene_render(context.scene, context.render_context))) {
            goto cleanup;
        }

        if (time_since_s(timespec) < FRAME_TIME_S) {
            usleep(FRAME_TIME_S - time_since_s(timespec));
        }
    }

cleanup:
    backend_destroy(context.render_context);

    return retval;
}
