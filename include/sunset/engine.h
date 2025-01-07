#pragma once

#include <stdbool.h>

#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/crypto.h"
#include "sunset/ecs.h"
#include "sunset/rman.h"
#include "sunset/events.h"
#include "sunset/vector.h"

typedef struct UIContext UIContext;

typedef void *PluginHandle;

struct EngineContext {
    vector(UIContext) ui_contexts;
    UIContext *active_ui;

    vector(PluginHandle) loaded_plugins;

    CommandBuffer cmdbuf;
    RenderContext render_context;
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

void engine_set_mouse_mode(EngineContext *engine_context, MouseMode mode);
