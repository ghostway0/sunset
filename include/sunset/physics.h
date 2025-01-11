#pragma once

#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>

#include "internal/utils.h"
#include "sunset/ecs.h"
#include "sunset/events.h"
#include "sunset/vector.h"

typedef struct EngineContext EngineContext;

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

struct PhysicsObject {
    enum physics_object_type type;

    vec3 velocity;
    vec3 acceleration;
    float damping;
    float mass;

    struct physics_material material;
} typedef PhysicsObject;

DECLARE_COMPONENT_ID(PhysicsObject);

struct CollisionPair {
    Index a;
    Index b;
} typedef CollisionPair;

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
        EventQueue *event_queue,
        float dt);

bool physics_move_object(struct scene *scene,
        struct object *object,
        vec3 direction,
        EventQueue *event_queue);

void physics_callback(EngineContext *engine_context,
        void *physics,
        Event /* engine tick */);

struct PhysicsState {
    enum physics_object_type type;
    struct physics_material material;

    enum physics_flags flags;

    vec3 acceleration;
    vec3 velocity;
    float damping;
    float mass;
} typedef PhysicsState;
