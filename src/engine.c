#include <dlfcn.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/errors.h"
#include "sunset/events.h"
#include "sunset/octree.h"
#include "sunset/rman.h"
#include "sunset/ui.h"
#include "sunset/utils.h"

#include "sunset/engine.h"

static float const FRAME_TIME_S = 1.0f / 60.0f;
static float const TICK_TIME_S = 1.0f / 60.0f;

static int engine_tick(EngineContext *context) {
    // send tick event
    event_queue_process_one(
            context, &context->event_queue, (Event){.type_id = SYSEV_TICK});

    // process all generated events
    event_queue_process(&context->event_queue, context);

    return 0;
}

// TODO: use the rman
/*
struct physics_context {
    struct scene *scene;
    struct physics physics;
    struct event_queue *event_queue;
};

void example_setup_physics(EngineContext *engine_context) {
    struct physics *physics = malloc(sizeof(struct physics));
    physics_init(physics);

    event_queue_add_handler(&engine_context->event_queue,
            SYSEV_TICK,
            (struct event_handler){.local_context = physics,
                    .handler_fn = physics_callback});
}

void example_destroy_physics(void *local_context) {
    struct physics *physics = local_context;
    physics_destroy(physics);
}
*/

typedef int (*PluginLoadFn)(EngineContext *context);
typedef int (*PluginUnloadFn)(EngineContext *context);

static int load_plugin(EngineContext *context, Plugin const *plugin) {
    // verify digest?

    void *handle = dlopen(plugin->object_path, RTLD_NOW);
    if (!handle) {
        // XXX:
        return -ERROR_IO;
    }

    PluginLoadFn load_fn = dlsym(handle, "plugin_load");

    return load_fn(context);
}

static int unload_plugin(EngineContext *context, void *handle) {
    PluginUnloadFn unload_fn = dlsym(handle, "plugin_unload");
    return unload_fn(context);
}

DECLARE_RESOURCE_ID(OcTree);

static int engine_setup(EngineContext *context, Game const *game) {
    int err;

    event_queue_init(&context->event_queue);
    rman_init(&context->rman);

    ecs_init(&context->world);

    cmdbuf_init(&context->cmdbuf, COMMAND_BUFFER_DEFAULT);

    if ((err = backend_setup(&context->render_context,
                 (RenderConfig){.window_width = 1920,
                         .window_height = 1080,
                         .window_title = "Test"}))) {
        return err;
    }

    for (size_t i = 0; i < vector_size(game->plugins); i++) {
        if ((err = load_plugin(context, &game->plugins[i]))) {
            return err;
        }
    }

    context->dt = FRAME_TIME_S;

    return 0;
}

void engine_destroy(EngineContext *context) {
    for (size_t i = 0; i < vector_size(context->loaded_plugins); i++) {
        unload_plugin(context, context->loaded_plugins[i]);
    }
}

int engine_run(Game const *game) {
    int retval = 0;
    EngineContext context;

    if ((retval = engine_setup(&context, game))) {
        return retval;
    }

    // TODO: figure out who's responsible for this init
    OcTree *octree = sunset_malloc(sizeof(OcTree));

    REGISTER_RESOURCE(&context.rman, /* rname = */ OcTree, octree);

    for (size_t i = 0; i < vector_size(game->plugins); i++) {
        load_plugin(&context, &game->plugins[i]);
    }

    struct timespec last_tick = get_time();

    while (!backend_should_stop(&context.render_context)) {
        struct timespec timespec = get_time();

        // TODO: handle input using backend
        // backend_capture_input(context.render_context,
        // &context.input_snippet);

        if (time_since_s(last_tick) >= TICK_TIME_S) {
            if ((retval = engine_tick(&context))) {
                goto cleanup;
            }

            last_tick = get_time();
        }

        // render

        if (time_since_s(timespec) < FRAME_TIME_S) {
            usleep(FRAME_TIME_S - time_since_s(timespec));
        }
    }

cleanup:
    backend_destroy(&context.render_context);

    return retval;
}
