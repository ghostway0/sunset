#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sunset/ecs.h"
#include "sunset/ecs_types.h"
#include "sunset/geometry.h"
#include "sunset/vector.h"

struct RenderConfig {
    size_t window_width, window_height;
    char const *window_title;

    size_t preferred_gpu;
    bool enable_vsync;
} typedef RenderConfig;

typedef struct Transform {
    AABB bounding_box;
    vec3 position;
    vec3 rotation;
    float scale;
} Transform;

typedef struct Renderable {
    // TODO: where should this Index live?
    Index mesh;
    Index texture;
} Renderable;

struct Chunk {
    AABB bounds;
    vector(Index) entities;
    size_t id;
} typedef Chunk;

enum WindowPoint {
    WINDOW_TOP_LEFT,
    WINDOW_TOP_RIGHT,
    WINDOW_BOTTOM_LEFT,
    WINDOW_BOTTOM_RIGHT,
} typedef WindowPoint;

DECLARE_COMPONENT_ID(Transform);
DECLARE_COMPONENT_ID(Renderable);

void calculate_model_matrix(Transform const *transform, mat4 model_matrix);
