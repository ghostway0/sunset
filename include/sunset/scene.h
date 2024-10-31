#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/geometry.h"
#include "sunset/octree.h"

struct transform {
    vec3 position;
    vec3 rotation;
    float scale;
};

struct physics_object {
    vec3 velocity;
    vec3 acceleration;
    float mass;
    float damping;
    bool should_fix;
};

struct object;

typedef void (*handler)(struct object *object);

enum keyboard_key {
    KEY_A,
    KEY_B,
    NUM_KEYBOARD_KEYS,
};

struct player_controller {
    handler handlers[NUM_KEYBOARD_KEYS];
};

struct ai_controller {};

enum controller_type {
    CONTROLLER_NONE,
    CONTROLLER_PLAYER,
    CONTROLLER_AI,
};

struct controller {
    enum controller_type type;
    union {
        struct player_controller player;
        struct ai_controller ai;
    };
};

struct object {
    struct physics_object physics;
    struct box bounding_box;
    struct transform transform;
    uint32_t mesh_id;
    struct compiled_texture *textures;
    size_t num_textures;
    struct material *materials;
    size_t num_materials;
    struct controller controller;

    struct object *parent;
    struct object **children;
    size_t num_children;
};

void object_move(struct object *object, vec3 direction);

enum light_type {
    LIGHT_DIRECTIONAL,
    LIGHT_SPOTLIGHT,
    LIGHT_POINT,
    LIGHT_AMBIENT,
};

struct light {
    enum light_type type;
    vec3 position;
    vec3 color;
    float intensity;
};

struct chunk {
    struct box bounds;
    struct object **objects;
    size_t num_objects;
    struct light *lights;
    size_t num_lights;
    size_t id;
};

struct scene {
    struct object *live_objects;
    size_t num_objects;
    struct oct_tree oct_tree;
    struct camera camera;
    struct image skybox;
    struct effect *effects;
    size_t num_effects;
};

void scene_init(struct camera camera,
        struct image skybox,
        struct effect *effects,
        size_t num_effects,
        struct box bounds,
        struct chunk *root_chunk,
        struct scene *scene_out);

bool position_within_box(vec3 position, struct box box);

struct chunk *get_chunk_for(struct scene const *scene, vec3 position);

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks);

void scene_move_camera(struct scene *scene, vec3 direction);

void scene_destroy(struct scene *scene);

struct chunk *scene_get_chunk_for(struct scene const *scene, vec3 position);

int scene_render(struct scene *scene, struct render_context *render_context);
