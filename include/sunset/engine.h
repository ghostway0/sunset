#pragma once

#include <stdbool.h>

#include <cglm/types.h>

#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/crypto.h"
#include "sunset/ecs.h"
#include "sunset/events.h"
#include "sunset/rman.h"
#include "sunset/vector.h"

typedef struct UIContext UIContext;

typedef void *PluginHandle;

typedef struct DebugInfo {
    float avg_frametime;
    vec3 direction;
} DebugInfo;

typedef struct EngineContext {
    vector(UIContext) ui_contexts;
    UIContext *active_ui;

    vector(PluginHandle) loaded_plugins;

    CommandBuffer cmdbuf;
    RenderContext render_context;

    EventQueue event_queue;
    ResourceManager rman;
    World world;

    Camera camera;

    float dt;

    DebugInfo debug_info;
} EngineContext;

typedef enum MouseMode {
    MOUSE_MODE_DISABLED,
    MOUSE_MODE_SHOW,
} MouseMode;

typedef struct Plugin {
    char const *object_path;
    // Digest expected_digest;
} Plugin;

typedef struct Game {
    World world;

    vector(Plugin) plugins;

    vector(char const *) resources;

    Signature sig;
} Game;

// external API

int engine_run(RenderConfig render_config, Game const *game);

void engine_set_mouse_mode(EngineContext *engine_context, MouseMode mode);
