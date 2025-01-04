#pragma once

#include <stdbool.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/crypto.h"
#include "sunset/ecs.h"
#include "sunset/physics.h"
#include "sunset/rman.h"
#include "sunset/vector.h"

typedef struct UIContext UIContext;
typedef struct EventQueue EventQueue;

typedef void *PluginHandle;

struct EngineContext {
    vector(UIContext) ui_contexts;
    UIContext *active_ui;

    vector(PluginHandle) loaded_plugins;

    struct command_buffer cmdbuf;
    struct render_context render_context;
    ResourceManager rman;

    EventQueue event_queue;
    World world;

    float dt;
} typedef EngineContext;

// engine context setup:
// 1. default ui_context
// 2. command buffer, render context
// 3. scene
// 4. event_queue
// 5. dt (60 fps default for now?)

enum MouseMode {
    MOUSE_MODE_DISABLED,
    MOUSE_MODE_SHOW,
} typedef MouseMode;

struct Plugin {
    char const *object_path;
    Digest expected_digest;
} typedef Plugin;

struct Game {
    AABB bounds;

    World world;

    vector(Plugin) plugins;

    vector(char const *) resources;

    Signature sig;
} typedef Game;

// external API

int engine_run(Game const *game);

int engine_load_scene(EngineContext *engine_context, struct scene *scene);

void engine_set_mouse_mode(EngineContext *engine_context, MouseMode mode);
