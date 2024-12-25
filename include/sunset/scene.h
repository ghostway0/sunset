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
#include "sunset/physics.h"

struct transform {
    vec3 position;
    vec3 rotation;
    float scale;
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

typedef void (*move_callback)(struct object *, vec3 direction);

struct object {
    struct physics_object physics;
    struct aabb bounding_box;
    struct transform transform;
    uint32_t mesh_id;
    uint32_t texture_id;
    struct material *materials;
    size_t num_materials;
    struct controller controller;

    char const *label;

    void *data;
    move_callback move_callback;

    struct object *parent;
    struct object **children;
    size_t num_children;
};

void object_rotate(struct object *object, vec3 rotation);

void object_destroy(struct object *object);

void object_set_velocity(struct object *object, vec3 velocity);

void object_scale_velocity(struct object *object, float factor);

void object_add_velocity(struct object *object, vec3 acceleration);

void object_calculate_model_matrix(
        struct object *object, mat4 model_matrix_out);

void object_rotate_velocity(struct object *object, float angle, vec3 axis);

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
    struct aabb bounds;
    vector(struct object *) objects;
    size_t id;
};

struct scene {
    vector(struct object) objects;
    vector(struct light) lights;
    vector(struct camera) cameras;
    struct octree octree;

    struct image skybox;
};

void scene_init(
        struct image skybox, struct aabb bounds, struct scene *scene_out);

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks);

void scene_destroy(struct scene *scene);

struct chunk *scene_get_chunk_for(struct scene const *scene, vec3 position);

int scene_render(struct scene *scene, struct render_context *render_context);

void scene_move_camera(
        struct scene *scene, size_t camera_index, vec3 direction);

void scene_rotate_camera(
        struct scene *scene, size_t camera_index, float x_angle, float y_angle);

void scene_move_object_with_parent(
        struct scene *scene, struct object *object, vec3 direction);

void scene_move_object(
        struct scene *scene, struct object *object, vec3 direction);

void scene_add_object(struct scene *scene, struct object object);

void scene_set_size(struct scene *scene, struct aabb new_bounds);
