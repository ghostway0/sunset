#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>
#include <string.h>

#include "cglm/types.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/octree.h"
#include "sunset/physics.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
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

[[maybe_unused]]
static void calculate_mtv(struct box a, struct box b, vec3 mtv_out) {
    vec3 overlap_min, overlap_max;

    assert(box_collide(&a, &b));

    glm_vec3_maxv(a.min, b.min, overlap_min);
    glm_vec3_minv(a.max, b.max, overlap_max);

    vec3 mtv = {0.0f, 0.0f, 0.0f};
    float overlap_x = overlap_max[0] - overlap_min[0];
    float overlap_y = overlap_max[1] - overlap_min[1];
    float overlap_z = overlap_max[2] - overlap_min[2];

    if (overlap_x < overlap_y && overlap_x < overlap_z) {
        mtv[0] = (a.min[0] < b.min[0]) ? -overlap_x : overlap_x;
    } else if (overlap_y < overlap_z) {
        mtv[1] = (a.min[1] < b.min[1]) ? -overlap_y : overlap_y;
    } else {
        mtv[2] = (a.min[2] < b.min[2]) ? -overlap_z : overlap_z;
    }

    glm_vec3_scale(mtv, 0.5f, mtv_out);
}

static struct physics_material combine_materials(
        struct physics_material a, struct physics_material b) {
    return (struct physics_material){
            .friction = a.friction * b.friction,
            .restitution = a.restitution * b.restitution,
    };
}

static void fix_collision(struct physics_object *a,
        bool fix_a,
        struct physics_object *b,
        bool fix_b) {
    assert(a->should_fix || b->should_fix);

    struct physics_material combined_material =
            combine_materials(a->material, b->material);

    if (fix_a && fix_b) {
        // v1' = (m1 - e * m2) * v1 + (1 + e) * m2 * v2 / (m1 + m2)
        // v2' = (1 + e) * m1 * v1 / (m1 + m2) + (m2 - e * m1) * v2
        // where v1 and v2 are the initial velocities of the two objects

        vec3 new_velocity;
        glm_vec3_scale(a->velocity,
                a->mass - combined_material.restitution * b->mass,
                new_velocity);

        vec3 other_velocity;
        glm_vec3_scale(b->velocity,
                (1.0f + combined_material.restitution) * b->mass,
                other_velocity);

        glm_vec3_add(new_velocity, other_velocity, new_velocity);
        glm_vec3_scale(new_velocity, 1.0f / (a->mass + b->mass), a->velocity);

        glm_vec3_scale(b->velocity,
                b->mass - combined_material.restitution * a->mass,
                new_velocity);

        glm_vec3_scale(a->velocity,
                (1.0f + combined_material.restitution) * a->mass,
                other_velocity);

        glm_vec3_add(new_velocity, other_velocity, new_velocity);
        glm_vec3_scale(new_velocity, 1.0f / (a->mass + b->mass), b->velocity);
    }

    if (!fix_b) {
        // b's mass is infinite
        glm_vec3_scale(a->velocity, -combined_material.restitution, b->velocity);
    }

    if (!fix_a) {
        // a's mass is infinite
        glm_vec3_scale(b->velocity, -combined_material.restitution, a->velocity);
    }
}

void physics_step(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue,
        float dt) {
    apply_constraint_forces(physics, dt);

    // for (size_t i = 0; i < vector_size(physics->objects); i++) {
    //     struct object *object = physics->objects[i];
    //
    //     glm_vec3_add(object->physics.acceleration,
    //             (vec3){0.0f, -9.81f, 0.0f},
    //             object->physics.acceleration);
    // }

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        glm_vec3_add(object->physics.velocity,
                object->physics.acceleration,
                object->physics.velocity);

        glm_vec3_scale(object->physics.velocity,
                1.0 - object->physics.damping,
                object->physics.velocity);

        vec3 velocity_scaled;
        glm_vec3_scale(object->physics.velocity, dt, velocity_scaled);

        vec3 moved;
        glm_vec3_add(object->transform.position, velocity_scaled, moved);

        // NOTE: when frame-rate is too low, this may
        // become a problem when 'spooky things at
        // a distance' occur.
        struct box path_box = object->bounding_box;
        box_extend_to(&path_box, moved);

        struct const_octree_iterator iterator;
        octree_const_iterator_init(&scene->oct_tree, &iterator);

        bool found_collision = false;

        while (iterator.current != NULL) {
            struct chunk *chunk = iterator.current->data;

            for (size_t j = 0; j < chunk->num_objects; j++) {
                struct object *other = chunk->objects[j];

                if (object == other) {
                    continue;
                }

                if (box_collide(&path_box, &other->bounding_box)) {
                    if (object->physics.should_fix || other->physics.should_fix) {
                        fix_collision(&object->physics,
                                object->physics.should_fix,
                                &other->physics,
                                other->physics.should_fix);

                        found_collision = true;
                    }

                    event_queue_push(event_queue,
                            (struct event){.type_id = SYSTEM_EVENT_COLLISION,
                                    .data.collision = (struct collision_event){
                                            .a = object, .b = other}});
                    break;
                }
            }

            octree_const_iterator_next(&iterator);
        }

        if (!found_collision) {
            box_translate(&object->bounding_box, velocity_scaled);
            glm_vec3_add(object->transform.position,
                    velocity_scaled,
                    object->transform.position);
        }
    }
}
