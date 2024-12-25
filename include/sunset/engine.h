#pragma once

#include <stdbool.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/physics.h"
#include "sunset/vector.h"

struct event_queue;
struct scene;
struct button;
struct ui_context;

struct rmanager {
    // map texture/mesh names to texture/mesh ids
    // fonts
    // images
    // atlases
};

struct engine_context {
    vector(struct ui_context) ui_contexts;
    struct ui_context *active_ui;

    struct command_buffer command_buffer;
    struct render_context render_context;
    struct rmanager rmanager;
    struct scene *scene;

    struct event_queue event_queue;

    float dt;
};

// engine context setup:
// 1. default ui_context
// 2. command buffer, render context
// 3. scene
// 4. event_queue
// 5. dt (60 fps default for now?)

enum mouse_mode {
    MOUSE_MODE_DISABLED,
    MOUSE_MODE_SHOW,
};

// a game is a collection of plugins.
// to load a game, setup engine_context based on the json and
// load all shared objects. call plugin_init within the objects
// with the engine_context.

struct game {
    // scene stuff (bounds, objects)

    // plugin names (vector)

    // textures, meshes, atlases
};

// external API

int engine_run(struct game *game);

int engine_load_scene(struct scene *scene);

void engine_set_mouse_mode(
        struct engine_context *engine_context, enum mouse_mode mode);
