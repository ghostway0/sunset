#pragma once

#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>

#include "sunset/events.h"
#include "sunset/vector.h"

enum system_event {
    SYSTEM_EVENT_COLLISION,
    SYSTEM_EVENT_MOUSE,
};

struct constraint {
    struct object *a;
    struct object *b;
    float distance;
};

struct physics_material {
    float restitution;
    float friction;
};

struct physics_object {
    vec3 velocity;
    vec3 acceleration;
    float mass;
    float damping;
    bool should_fix;

    struct physics_material material;
};

static_assert(
        sizeof(struct collision_event) <= 60, "collision_event too large");

struct physics {
    vector(struct object *) objects;
    vector(struct constraint) constraints;
};

void physics_init(struct physics *physics);

// FIXME: consistent naming
void physics_free(struct physics *physics);

void physics_add_object(struct physics *physics, struct object *object);

void physics_add_constraint(struct physics *physics,
        struct object *a,
        struct object *b,
        float distance);

struct scene;

void physics_step(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue,
        float dt);
