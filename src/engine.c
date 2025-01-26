#include <dlfcn.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <cglm/types.h>
#include <log.h>

#include "internal/mem_utils.h"
#include "internal/time_utils.h"
#include "sunset/backend.h"
#include "sunset/bitmask.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/errors.h"
#include "sunset/events.h"
#include "sunset/fonts.h"
#include "sunset/images.h"
#include "sunset/input.h"
#include "sunset/octree.h"
#include "sunset/render.h"
#include "sunset/rman.h"
#include "sunset/ui.h"
#include "sunset/vector.h"

#include "sunset/engine.h"

static float const FRAME_TIME_S = 1.0f / 60.0f;

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

    input_setup(context);

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
    event_queue_process_one(context,
            &context->event_queue,
            (Event){.event_id = SYSTEM_EVENT_TICK});

    backend_generate_input_events(&context->render_context);

    event_queue_process(&context->event_queue, context);

    return 0;
}

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

            cmdbuf_add_multiple(cmdbuf,
                    renderable->commands,
                    vector_size(renderable->commands));
        }

        worldit_advance(&it);
    }
}

static void camera_viewport_handler(
        EngineContext *, void *ctx, Event const event) {
    Camera *camera = ctx;

    Point *viewport_dims = (Point *)event.data;

    camera_set_aspect_ratio(camera, viewport_dims->y / viewport_dims->x);
}

static void clicked(EngineContext *) {
    log_debug("clicked!");
}

static void thing(
        EngineContext *engine_context, void *, Event const event) {
    Key *key = (Key *)event.data;

    if (*key == KEY_ESCAPE) {
        uint32_t *focus =
                rman_get(&engine_context->rman, RESOURCE_ID(input_focus));
        *focus = FOCUS_UI;
        backend_show_mouse(&engine_context->render_context);
    }
}

static void thing2(
        EngineContext *engine_context, void *, Event const event) {
    MouseClickEvent *click = (MouseClickEvent *)event.data;
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (click->button == MOUSE_BUTTON_LEFT && *focus == FOCUS_NULL) {
        // focus engine
        *focus = 3;
        backend_hide_mouse(&engine_context->render_context);
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

    // temporary
    event_queue_add_handler(&context.event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){.handler_fn = thing2, .local_context = NULL});
    event_queue_add_handler(&context.event_queue,
            SYSTEM_EVENT_KEY_PRESSED,
            (EventHandler){.handler_fn = thing, .local_context = NULL});

    Font font;
    load_font_psf2("font.psf", &font);

    UIContext uictx = {};
    ui_init(&uictx);

    Widget *widget1 = sunset_malloc(sizeof(Widget));
    *widget1 = (Widget){.tag = WIDGET_TEXT,
            .text = {"test", &font, 24},
            .active = true,
            .bounds = {100, 80, 100, 10},
            .parent = NULL,
            .children = NULL};
    ui_add_widget(uictx.root, widget1);

    Widget *widget2 = sunset_malloc(sizeof(Widget));
    *widget2 = (Widget){.tag = WIDGET_BUTTON,
            .button = {.clicked_callback = clicked},
            .bounds = {100, 100, 100, 100},
            .active = true,
            .parent = NULL,
            .style = {.color = COLOR_WHITE, .solid = false},
            .children = NULL};
    ui_add_widget(uictx.root, widget2);

    Widget *widget3 = sunset_malloc(sizeof(Widget));
    *widget3 = (Widget){.tag = WIDGET_TEXT,
            .text = {"fun", &font, 24},
            .bounds = {30, 30, 0, 0},
            .style = {.relative = true},
            .active = true};
    ui_add_widget(widget2, widget3);

    Widget *widget4 = sunset_malloc(sizeof(Widget));
    *widget4 = (Widget){.tag = WIDGET_INPUT,
            .input = {.text = NULL, .font = &font, 24},
            .bounds = {30, 30, 100, 100},
            .style = {.color = COLOR_WHITE},
            .active = true};
    ui_add_widget(uictx.root, widget4);

    context.active_ui = &uictx;
    // temporary

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
