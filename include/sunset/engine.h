#pragma once

#include <stdbool.h>

#include "geometry.h"
#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/physics.h"
#include "sunset/vector.h"
#include "vector.h"

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

struct game_config {
    struct rect bounds;
    // game objects
    // scene stuff (bounds, objects)

    // the scene can be saved

    // resources
    vector(char const *) plugins;
    vector(char const *) atlases;
    vector(char const *) meshes;
};

// external API

int engine_run(struct game_config *game);

int engine_load_scene(
        struct engine_context *engine_context, struct scene *scene);

void engine_set_mouse_mode(
        struct engine_context *engine_context, enum mouse_mode mode);
