#include <dlfcn.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "camera.h"
#include "cglm/types.h"
#include "internal/time_utils.h"
#include "sunset/backend.h"
#include "sunset/bitmask.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/errors.h"
#include "sunset/events.h"
#include "sunset/octree.h"
#include "sunset/render.h"
#include "sunset/rman.h"
#include "sunset/ui.h"

#include "sunset/engine.h"

static float const FRAME_TIME_S = 1.0f / 60.0f;
static float const TICK_TIME_S = 1.0f / 60.0f;

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
            SYSTEM_EVENT_TICK,
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

static int load_plugin(EngineContext *context,
        Plugin const *plugin,
        PluginHandle *handle_out) {
    int err;
    // verify digest?

    void *handle = dlopen(plugin->object_path, RTLD_NOW);
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
        return -ERROR_INVALID_PLUGIN;
    }

    return unload_fn(context);
}

DECLARE_RESOURCE_ID(OcTree);

static int engine_setup(EngineContext *context, Game const *game) {
    int err;

    // main system communication
    event_queue_init(&context->event_queue);
    rman_init(&context->rman);
    ecs_init(&context->world);

    // backend setup

    cmdbuf_init(&context->cmdbuf, COMMAND_BUFFER_DEFAULT);

    // TODO: get render config
    if ((err = backend_setup(&context->render_context,
                 (RenderConfig){.window_width = 1920,
                         .window_height = 1080,
                         .window_title = "Test"}))) {
        return err;
    }

    // engine setup

    vector_init(context->loaded_plugins);
    vector_resize(context->loaded_plugins, vector_size(game->plugins));

    for (size_t i = 0; i < vector_size(game->plugins); i++) {
        if ((err = load_plugin(context,
                     &game->plugins[i],
                     &context->loaded_plugins[i]))) {
            return err;
        }
    }

    ui_setup(context);

    context->dt = FRAME_TIME_S;

    return 0;
}

void engine_destroy(EngineContext *context) {
    for (size_t i = 0; i < vector_size(context->loaded_plugins); i++) {
        unload_plugin(context, context->loaded_plugins[i]);
    }
}

// XXX: where should this be?
void *octree_init_resource(void) {
    OcTree *octree = sunset_malloc(sizeof(OcTree));

    return octree;
}

static int engine_tick(EngineContext *context) {
    // send tick event
    event_queue_process_one(context,
            &context->event_queue,
            (Event){.event_id = SYSTEM_EVENT_TICK});

    // process all generated events
    event_queue_process(&context->event_queue, context);

    return 0;
}

// static int render_object(
//         struct object *object, struct command_buffer *command_buffer) {
//     mat4 model_matrix;
//     object_calculate_model_matrix(object, model_matrix);
//
//     command_buffer_add_mesh(command_buffer,
//             object->mesh_id,
//             object->texture_id,
//             model_matrix);
//
//     return 0;
// }

void render_world(
        World const *world, Camera const *camera, CommandBuffer *cmdbuf) {
    Bitmask mask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &mask);
    bitmask_set(&mask, COMPONENT_ID(Renderable));
    bitmask_set(&mask, COMPONENT_ID(Transform));

    WorldIterator it = worldit_create(world, mask);

    while (worldit_is_valid(&it)) {
        Renderable *renderable =
                worldit_get_component(&it, COMPONENT_ID(Renderable));
        Transform *transform =
                worldit_get_component(&it, COMPONENT_ID(Transform));

        // HACK: when I transition to my own math library, const
        // when unmutable would be a rule
        if (camera_box_within_frustum(
                    (Camera *)camera, transform->bounding_box)) {
            mat4 model;
            calculate_model_matrix(transform, model);

            cmdbuf_add_mesh(
                    cmdbuf, renderable->mesh, renderable->texture, model);
        }

        worldit_advance(&it);
    }
}

int engine_run(Game const *game) {
    int retval = 0;
    EngineContext context = {0};

    if ((retval = engine_setup(&context, game))) {
        return retval;
    }

    rman_get_or_init(&context.rman, OcTree, octree_init_resource);

    Time last_tick = get_time();

    while (!backend_should_stop(&context.render_context)) {
        Time timespec = get_time();

        // TODO: handle input using backend
        // InputState instate =
        // backend_capture_input(context.render_context);
        // event_queue_process_one(..., SYSTEM_EVENT_INPUT_SNIPPET);
        // ? event_queue_process()

        if (time_since_s(last_tick) >= TICK_TIME_S) {
            if ((retval = engine_tick(&context))) {
                goto cleanup;
            }

            last_tick = get_time();
        }

        // FIXME: add camera when I have one
        render_world(&context.world, /*camera*/ NULL, &context.cmdbuf);

        if (time_since_s(timespec) < FRAME_TIME_S) {
            usleep(FRAME_TIME_S - time_since_s(timespec));
        }
    }

cleanup:
    backend_destroy(&context.render_context);

    return retval;
}
