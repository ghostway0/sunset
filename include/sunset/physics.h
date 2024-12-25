#pragma once

#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>

#include "sunset/events.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

struct scene;

struct constraint {
    struct object *a;
    struct object *b;
    float distance;
};

struct physics_material {
    float restitution;
    float friction;
};

enum physics_object_type {
    /// doesn't affect other objects, just checks for collisions
    PHYSICS_OBJECT_COLLIDER,
    /// objects that don't get fixed after a collision, but do affect other
    /// objects
    PHYSICS_OBJECT_INFINITE,
    /// regular objects
    PHYSICS_OBJECT_REGULAR,
};

enum physics_flags {
    PHYSICS_FLAGS_APPLY_GRAVITY = sunset_flag(0),
};

struct physics_object {
    enum physics_object_type type;

    vec3 velocity;
    vec3 acceleration;
    float damping;
    float mass;

    struct physics_material material;
};

struct collision_pair {
    struct object *a;
    struct object *b;
};

struct physics {
    vector(struct object *) objects;
    vector(struct constraint) constraints;
    vector(struct collision_pair) collision_pairs;
};

void physics_init(struct physics *physics);

void physics_destroy(struct physics *physics);

void physics_add_object(struct physics *physics,
        struct object *object,
        enum physics_flags flags);

void physics_add_constraint(struct physics *physics,
        struct object *a,
        struct object *b,
        float distance);

void physics_step(struct physics *physics,
        struct scene *scene,
        struct event_queue *event_queue,
        float dt);

bool physics_move_object(struct scene *scene,
        struct object *object,
        vec3 direction,
        struct event_queue *event_queue);

void physics_callback(struct engine_context *engine_context,
        void *physics,
        struct event /* engine tick */);
