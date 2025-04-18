#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sunset/ecs.h"
#include "sunset/ecs_types.h"
#include "sunset/geometry.h"
#include "sunset/vector.h"

typedef struct CommandBuffer CommandBuffer;
typedef struct EngineContext EngineContext;
typedef struct Command Command;
typedef struct Camera Camera;

typedef struct RenderConfig {
    size_t window_width, window_height;
    char const *window_title;

    size_t preferred_gpu;
    bool enable_vsync;
} RenderConfig;

typedef struct Transform {
    AABB bounding_box;
    vec3 position;
    vec3 rotation;
    float scale;
    bool dirty;

    EntityPtr parent;
    vector(EntityPtr) children;

    mat4 cached_model;
} Transform;

typedef struct Chunk {
    AABB bounds;
    vector(Index) entities;
    size_t id;
} Chunk;

typedef enum WindowPoint {
    WINDOW_TOP_LEFT,
    WINDOW_TOP_RIGHT,
    WINDOW_BOTTOM_LEFT,
    WINDOW_BOTTOM_RIGHT,
} WindowPoint;

extern DECLARE_COMPONENT_ID(Transform);

void calculate_model_matrix(
        World *world, EntityPtr eptr, mat4 model_matrix);

void render_world(World /*const*/ *world,
        Camera const *camera,
        CommandBuffer *cmdbuf);

void render_setup(EngineContext *engine_context);

void entity_move(World *world, EntityPtr eptr, vec3 offset);

void entity_get_abspos(World *world, EntityPtr eptr, vec3 out);
