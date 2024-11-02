#pragma once

#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>

#include "sunset/events.h"
#include "sunset/scene.h"
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

void physics_step(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue,
        float dt);
