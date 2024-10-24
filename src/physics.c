#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/octree.h"
#include "sunset/physics.h"
#include "sunset/scene.h"
#include "sunset/vector.h"

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

static void update_collisions(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue) {
    struct const_octree_iterator iterator;
    octree_const_iterator_init(&scene->oct_tree, &iterator);

    while (iterator.current != NULL) {
        struct chunk *chunk = iterator.current->data;

        for (size_t i = 0; i < chunk->num_objects; i++) {
            struct object *object = chunk->objects[i];

            for (size_t j = i + 1; j < vector_size(physics->objects); j++) {
                struct object *other = physics->objects[j];

                if (object == other) {
                    continue;
                }

                if (box_collide(&object->bounding_box, &other->bounding_box)) {
                    struct collision_event collision_event = {.a = object, .b = other};

                    struct event event;
                    event.type_id = SYSTEM_EVENT_COLLISION;
                    memcpy(event.data,
                            &collision_event,
                            sizeof(collision_event));

                    event_queue_push(event_queue, &event);
                }
            }
        }

        octree_const_iterator_next(&iterator);
    }
}

void physics_step(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue,
        float dt) {
    apply_constraint_forces(physics, dt);

    update_collisions(physics, scene, event_queue);

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        glm_vec3_add(object->physics.acceleration,
                (vec3){0.0f, -9.81f, 0.0f},
                object->physics.acceleration);
    }

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
