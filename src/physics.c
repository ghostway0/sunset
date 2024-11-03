#include <stddef.h>

#include <cglm/vec3.h>
#include <stdint.h>
#include <string.h>

#include "cglm/types.h"
#include "log.h"
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

    glm_vec3_zero(mtv_out);

    glm_vec3_maxv(a.min, b.min, overlap_min);
    glm_vec3_minv(a.max, b.max, overlap_max);

    float overlap_x = overlap_max[0] - overlap_min[0];
    float overlap_y = overlap_max[1] - overlap_min[1];
    float overlap_z = overlap_max[2] - overlap_min[2];

    if (overlap_x < overlap_y && overlap_x < overlap_z) {
        mtv_out[0] = (a.min[0] < b.min[0]) ? -overlap_x : overlap_x;
    } else if (overlap_y < overlap_z) {
        mtv_out[1] = (a.min[1] < b.min[1]) ? -overlap_y : overlap_y;
    } else {
        mtv_out[2] = (a.min[2] < b.min[2]) ? -overlap_z : overlap_z;
    }
}

static struct physics_material combine_materials(
        struct physics_material a, struct physics_material b) {
    return (struct physics_material){
            .friction = a.friction * b.friction,
            .restitution = a.restitution * b.restitution,
    };
}

static void fix_combined_velocities(
        struct object *a, bool fix_a, struct object *b, bool fix_b) {
    struct physics_object *a_attr = &a->physics;
    struct physics_object *b_attr = &b->physics;

    assert(a_attr->should_fix || b_attr->should_fix);

    struct physics_material combined_material =
            combine_materials(a_attr->material, b_attr->material);

    if (fix_a && fix_b) {
        // v1' = (m1 - e * m2) * v1 + (1 + e) * m2 * v2 / (m1 + m2)
        // v2' = (1 + e) * m1 * v1 / (m1 + m2) + (m2 - e * m1) * v2
        // where v1 and v2 are the initial velocities of the two objects

        vec3 new_velocity;
        glm_vec3_scale(a_attr->velocity,
                a_attr->mass - combined_material.restitution * b_attr->mass,
                new_velocity);

        vec3 other_velocity;
        glm_vec3_scale(b_attr->velocity,
                (1.0f + combined_material.restitution) * b_attr->mass,
                other_velocity);

        glm_vec3_add(new_velocity, other_velocity, new_velocity);
        glm_vec3_scale(new_velocity,
                1.0f / (a_attr->mass + b_attr->mass),
                a_attr->velocity);

        glm_vec3_scale(b_attr->velocity,
                b_attr->mass - combined_material.restitution * a_attr->mass,
                new_velocity);

        glm_vec3_scale(a_attr->velocity,
                (1.0f + combined_material.restitution) * a_attr->mass,
                other_velocity);

        glm_vec3_add(new_velocity, other_velocity, new_velocity);
        glm_vec3_scale(new_velocity,
                1.0f / (a_attr->mass + b_attr->mass),
                new_velocity);

        object_set_velocity(a, new_velocity);
        object_set_velocity(b, new_velocity);
    }

    if (!fix_b) {
        // b's mass is infinite
        object_scale_velocity(a, -combined_material.restitution);
    }

    if (!fix_a) {
        // a's mass is infinite
        object_scale_velocity(b, -combined_material.restitution);
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

        object_add_velocity(object, object->physics.acceleration);

        object_scale_velocity(object, 1.0f - object->physics.damping);

        vec3 velocity_scaled, moved;

        glm_vec3_scale(object->physics.velocity, dt, velocity_scaled);
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

                if (box_collide(&object->bounding_box, &other->bounding_box)) {
                    vec3 mtv;
                    calculate_mtv(
                            object->bounding_box, other->bounding_box, mtv);

                    float scale = object->physics.should_fix
                                    && other->physics.should_fix
                            ? 0.5f
                            : 1.0f;

                    log_debug("MTV: %f %f %f", mtv[0], mtv[1], mtv[2]);

                    glm_vec3_scale(mtv, scale, mtv);

                    if (object->physics.should_fix) {
                        object_move(object, mtv);
                    }

                    if (other->physics.should_fix) {
                        glm_vec3_negate(mtv);
                        object_move(other, mtv);
                    }
                }

                if (box_collide(&path_box, &other->bounding_box)) {
                    if (object->physics.should_fix
                            || other->physics.should_fix) {
                        fix_combined_velocities(object,
                                object->physics.should_fix,
                                other,
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
            object_move(object, velocity_scaled);
        }
    }
}
