#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>
#include <string.h>

#include "sunset/events.h"
#include "sunset/physics.h"
#include "sunset/scene.h"
#include "sunset/vector.h"

enum system_event {
    SYSTEM_EVENT_COLLISION,
};

struct constraint {
    struct object *a;
    struct object *b;
    float distance;
};

struct collision_event {
    struct object *a;
    struct object *b;
};

struct physics {
    vector(struct object *) objects;
    vector(struct constraint) constraints;
};

void physics_init(struct physics *physics) {
    vector_init(physics->objects, struct object *);
    vector_init(physics->constraints, struct constraint);
}

void physics_free(struct physics *physics) {
    vector_free(physics->objects);
    vector_free(physics->constraints);
}

void physics_add_object(struct physics *physics, struct object *object) {
    vector_append(physics->objects, object);
}

void physics_add_constraint(struct physics *physics,
        struct object *a,
        struct object *b,
        float distance) {
    vector_append(physics->constraints, ((struct constraint){a, b, distance}));
}

static void apply_constraint_forces(struct physics const *physics, float dt) {
    for (size_t i = 0; i < vector_size(physics->constraints); i++) {
        struct constraint constraint = physics->constraints[i];
        struct object *a = constraint.a;
        struct object *b = constraint.b;

        vec3 direction;
        glm_vec3_sub(b->transform.position, a->transform.position, direction);

        float current_distance = glm_vec3_norm(direction);
        float diff = current_distance - constraint.distance;
        float correction = diff / 2.0f;

        vec3 correction_vector;
        glm_vec3_scale(direction, correction, correction_vector);

        glm_vec3_add(a->transform.position,
                correction_vector,
                a->transform.position);
        glm_vec3_sub(b->transform.position,
                correction_vector,
                b->transform.position);

        vec3 velocity;
        glm_vec3_sub(b->physics.velocity, a->physics.velocity, velocity);

        float velocity_diff = glm_vec3_norm(velocity);
        float velocity_correction = velocity_diff / 2.0f;

        vec3 velocity_correction_vector;
        glm_vec3_scale(
                velocity, velocity_correction * dt, velocity_correction_vector);

        glm_vec3_add(a->physics.velocity,
                velocity_correction_vector,
                a->physics.velocity);
        glm_vec3_sub(b->physics.velocity,
                velocity_correction_vector,
                b->physics.velocity);

        vec3 acceleration;
        glm_vec3_sub(
                b->physics.acceleration, a->physics.acceleration, acceleration);

        float acceleration_diff = glm_vec3_norm(acceleration);
        float acceleration_correction = acceleration_diff / 2.0f;

        vec3 acceleration_correction_vector;
        glm_vec3_scale(acceleration,
                acceleration_correction * dt,
                acceleration_correction_vector);

        glm_vec3_add(a->physics.acceleration,
                acceleration_correction_vector,
                a->physics.acceleration);
        glm_vec3_sub(b->physics.acceleration,
                acceleration_correction_vector,
                b->physics.acceleration);
    }
}

void physics_step(struct physics const *physics,
        struct event_queue *event_queue,
        float dt) {
    apply_constraint_forces(physics, dt);

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        glm_vec3_add(object->physics.acceleration,
                (vec3){0.0f, -9.81f, 0.0f},
                object->physics.acceleration);
    }

    // FIXME: should really do this only per chunk
    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *a = physics->objects[i];

        for (size_t j = i + 1; j < vector_size(physics->objects); j++) {
            struct object *b = physics->objects[j];

            if (bounding_box_collide(&a->bounding_box, &b->bounding_box)) {
                struct collision_event collision_event = {a, b};
                struct event event;

                memcpy(event.data, &collision_event, sizeof(collision_event));
                event.type_id = SYSTEM_EVENT_COLLISION;

                event_queue_push(event_queue, event);
            }
        }
    }
}

void physics_update(struct physics const *physics,
        struct event_queue *event_queue,
        float dt) {
    physics_step(physics, event_queue, dt);

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        glm_vec3_add(object->physics.velocity,
                object->physics.acceleration,
                object->physics.velocity);

        glm_vec3_scale(
                object->physics.velocity, 0.99f, object->physics.velocity);

        glm_vec3_add(object->transform.position,
                object->physics.velocity,
                object->transform.position);
    }
}
