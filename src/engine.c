#include <stdint.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/events.h"
#include "sunset/scene.h"
#include "sunset/ui.h"
#include "sunset/utils.h"

#include "sunset/engine.h"

static float const FRAME_TIME_S = 1.0f / 60.0f;

// engine tick:
// 1. send SYSTEM_EVENT_TICK event
// 2. zones??
// 3. ?????
// 4. profit
// should this be done _every_ frame?

void engine_setup(struct engine_context *context) {
    unused(context);
}

static int engine_tick(struct engine_context *context) {
    // send tick event
    event_queue_process_one(context,
            &context->event_queue,
            (struct event){.type_id = SYSTEM_EVENT_TICK});

    // process all generated events
    event_queue_process(&context->event_queue, context);

    return 0;
}

/*
struct physics_context {
    struct scene *scene;
    struct physics physics;
    struct event_queue *event_queue;
};

void example_setup_physics(struct engine_context *engine_context) {
    struct physics *physics = malloc(sizeof(struct physics));
    physics_init(physics);

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_TICK,
            (struct event_handler){
                    .local_context = physics, .handler_fn = physics_callback});
}

void example_destroy_physics(void *local_context) {
    struct physics *physics = local_context;
    physics_destroy(physics);
}
*/

int engine_run(struct game *game) {
    unused(game);
    int retval = 0;

    struct engine_context context = {};
    event_queue_init(&context.event_queue);
    // TODO: setup

    if ((retval = backend_setup(&context.render_context,
                 (struct render_config){.window_width = 1920,
                         .window_height = 1080,
                         .window_title = "Test"}))) {
        return retval;
    }

    while (!backend_should_stop(&context.render_context)) {
        struct timespec timespec = get_time();

        // TODO: handle input using backend
        // backend_generate_input_events(context.render_context);

        if ((retval = engine_tick(&context))) {
            goto cleanup;
        }

        if ((retval = scene_render(context.scene, &context.render_context))) {
            goto cleanup;
        }

        if (time_since_s(timespec) < FRAME_TIME_S) {
            usleep(FRAME_TIME_S - time_since_s(timespec));
        }
    }

cleanup:
    backend_destroy(&context.render_context);

    return retval;
}
