#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <cglm/types.h>
#include <log.h>

#include "internal/time_utils.h"
#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/errors.h"
#include "sunset/events.h"
#include "sunset/input.h"
#include "sunset/render.h"
#include "sunset/rman.h"
#include "sunset/ui.h"
#include "sunset/vector.h"

#include "sunset/engine.h"

static float const FRAME_TIME_S = 1.0f / 60.0f;

typedef int (*PluginLoadFn)(EngineContext *context);
typedef int (*PluginUnloadFn)(EngineContext *context);

static int load_plugin(EngineContext *context,
        Plugin const *plugin,
        PluginHandle *handle_out) {
    int err;
    // verify digest?

    void *handle = dlopen(plugin->object_path, RTLD_LAZY);

    if (!handle) {
        // XXX:
        return -ERROR_IO;
    }

    PluginLoadFn load_fn = dlsym(handle, "plugin_load");

    if (!load_fn) {
        return -ERROR_INVALID_PLUGIN;
    }

    if ((err = load_fn(context))) {
        return err;
    }

    *handle_out = handle;

    return 0;
}

static int unload_plugin(EngineContext *context, void *handle) {
    PluginUnloadFn unload_fn = dlsym(handle, "plugin_unload");

    if (!unload_fn) {
        return 0;
    }

    return unload_fn(context);
}

DECLARE_RESOURCE_ID(OcTree);

static int engine_setup(EngineContext *context,
        RenderConfig render_config,
        Game const *game) {
    int err;

    // main system communication
    event_queue_init(&context->event_queue);
    rman_init(&context->rman);
    ecs_init(&context->world);

    // backend setup

    cmdbuf_init(&context->cmdbuf, COMMAND_BUFFER_DEFAULT);

    if ((err = backend_setup(&context->render_context,
                 &context->event_queue,
                 render_config))) {
        return err;
    }

    camera_init(
            (CameraState){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (CameraOptions){
                    0.1f,
                    100.0f,
                    45.0f,
                    (float)render_config.window_height
                            / (float)render_config.window_width,
            },
            &context->camera);

    // engine setup

    ui_setup(context);

    input_setup(context);

    context->dt = FRAME_TIME_S;

    vector_init(context->loaded_plugins);
    vector_resize(context->loaded_plugins, vector_size(game->plugins));

    for (size_t i = 0; i < vector_size(game->plugins); i++) {
        if ((err = load_plugin(context,
                     &game->plugins[i],
                     &context->loaded_plugins[i]))) {
            return err;
        }
    }

    return 0;
}

static int engine_tick(EngineContext *context) {
    event_queue_process_one(context,
            &context->event_queue,
            (Event){.event_id = SYSTEM_EVENT_TICK});

    backend_generate_input_events(&context->render_context);

    event_queue_process(&context->event_queue, context);

    return 0;
}

static void camera_viewport_handler(
        EngineContext *, void *ctx, Event const event) {
    Camera *camera = ctx;

    Point *viewport_dims = (Point *)event.data;

    camera_set_aspect_ratio(camera, viewport_dims->y / viewport_dims->x);
}

void engine_destroy(EngineContext *context) {
    for (size_t i = 0; i < vector_size(context->loaded_plugins); i++) {
        unload_plugin(context, context->loaded_plugins[i]);
    }
}

int engine_run(RenderConfig render_config, Game const *game) {
    int retval = 0;
    EngineContext context = {0};

    if ((retval = engine_setup(&context, render_config, game))) {
        return retval;
    }

    // FIXME: this is not really nice
    event_queue_add_handler(&context.event_queue,
            SYSTEM_EVENT_VIEWPORT_CHANGED,
            (EventHandler){.handler_fn = camera_viewport_handler,
                    .local_context = &context.camera});

    while (!backend_should_stop(&context.render_context)) {
        Time timespec = get_time();

        if ((retval = engine_tick(&context))) {
            goto cleanup;
        }

        // multi camera?
        render_world(&context.world, &context.camera, &context.cmdbuf);

        [[maybe_unused]]
        uint64_t frame_time = time_since_us(timespec);

        backend_draw(&context.render_context,
                &context.cmdbuf,
                context.camera.view_matrix,
                context.camera.projection_matrix);

        if (time_since_s(timespec) < FRAME_TIME_S) {
            usleep(FRAME_TIME_S - time_since_s(timespec));
        }
    }

cleanup:
    backend_destroy(&context.render_context);

    return retval;
}
