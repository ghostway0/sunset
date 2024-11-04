#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include <log.h>

#include "sunset/events.h"
#include "sunset/geometry.h"
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

    if (object->physics.should_fix) {
        glm_vec3_sub(object->physics.acceleration,
                (vec3){0.0f, 0.01f, 0.0f},
                object->physics.acceleration);
    }
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

static void fix_combined_velocities(struct object *a, struct object *b) {
    struct physics_object *a_attr = &a->physics;
    struct physics_object *b_attr = &b->physics;

    assert(a_attr->should_fix || b_attr->should_fix);

    struct physics_material combined_material =
            combine_materials(a_attr->material, b_attr->material);

    vec3 closest_point_a, closest_point_b;
    box_closest_point(&a->bounding_box, b->transform.position, closest_point_a);
    box_closest_point(&b->bounding_box, a->transform.position, closest_point_b);

    vec3 collision_normal;
    glm_vec3_sub(closest_point_a, closest_point_b, collision_normal);
    glm_vec3_normalize(collision_normal);

    if (a_attr->should_fix && b_attr->should_fix) {
        float v1_normal = glm_vec3_dot(a_attr->velocity, collision_normal);
        float v2_normal = glm_vec3_dot(b_attr->velocity, collision_normal);

        float m1 = a_attr->mass;
        float m2 = b_attr->mass;
        float e = combined_material.restitution;

        float new_v1_normal =
                (v1_normal * (m1 - e * m2) + v2_normal * (1 + e) * m2)
                / (m1 + m2);
        float new_v2_normal =
                (v2_normal * (m2 - e * m1) + v1_normal * (1 + e) * m1)
                / (m1 + m2);

        vec3 delta_v1, delta_v2;
        glm_vec3_scale(collision_normal, new_v1_normal - v1_normal, delta_v1);
        glm_vec3_scale(collision_normal, new_v2_normal - v2_normal, delta_v2);

        object_add_velocity(a, delta_v1);
        object_add_velocity(b, delta_v2);

        return;
    }

    if (a_attr->should_fix) {
        // b's mass is infinite
        vec3 v1_normal, v1_tangent;

        glm_vec3_proj(a_attr->velocity, collision_normal, v1_normal);
        glm_vec3_sub(a_attr->velocity, v1_normal, v1_tangent);
        log_debug("a should fix %s " vec3_format,
                a->label,
                vec3_args(collision_normal));

        glm_vec3_scale(collision_normal,
                -glm_vec3_norm(v1_normal) * combined_material.restitution,
                v1_normal);

        log_debug("%s " vec3_format, a->label, vec3_args(v1_normal));

        object_set_velocity(a, v1_normal);
        object_add_velocity(a, v1_tangent);
    } else if (b_attr->should_fix) {
        // a's mass is infinite
        vec3 v2_normal, v2_tangent;

        glm_vec3_proj(b_attr->velocity, collision_normal, v2_normal);
        glm_vec3_sub(b_attr->velocity, v2_normal, v2_tangent);

        glm_vec3_scale(collision_normal,
                glm_vec3_norm(v2_normal) * combined_material.restitution,
                v2_normal);

        object_set_velocity(b, v2_normal);
        object_add_velocity(b, v2_tangent);
    }
}

uint64_t hash_ptr(struct object **ptr) {
    return (uint64_t)*ptr;
}

void physics_step(struct physics const *physics,
        struct scene const *scene,
        struct event_queue *event_queue,
        float dt) {
    apply_constraint_forces(physics, dt);

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        object_add_velocity(object, object->physics.acceleration);

        vec3 velocity_scaled, moved;

        glm_vec3_scale(object->physics.velocity, dt, velocity_scaled);
        glm_vec3_add(object->transform.position, velocity_scaled, moved);

        // NOTE: when frame-rate is too low, this may
        // become a problem when 'spooky things at
        // a distance' occur.
        struct box path_box = object->bounding_box;
        box_extend_to(&path_box, moved);

        bool found_collision = false;
        struct chunk *chunk = scene_get_chunk_for(scene, moved);

        for (size_t j = 0; j < chunk->num_objects; j++) {
            struct object *other = chunk->objects[j];

            if (object == other) {
                continue;
            }

            if (box_collide(&path_box, &other->bounding_box)) {
                log_debug("Collision detected between %s and %s velocity %f",
                        object->label,
                        other->label,
                        glm_vec3_norm(object->physics.velocity));

                struct event event = {
                        .type_id = SYSTEM_EVENT_COLLISION,
                        .data.collision =
                                {
                                        .a = object,
                                        .b = other,
                                },
                };

                glm_vec3_copy(object->physics.velocity,
                        event.data.collision.a_velocity);
                glm_vec3_copy(other->physics.velocity,
                        event.data.collision.b_velocity);

                // if (object->physics.should_fix || other->physics.should_fix)
                // {
                fix_combined_velocities(object, other);

                found_collision = true;
                // }

                event_queue_push(event_queue, event);

                break;
            }

            if (box_collide(&object->bounding_box, &other->bounding_box)) {
                vec3 mtv;
                calculate_mtv(object->bounding_box, other->bounding_box, mtv);

                float scale =
                        object->physics.should_fix && other->physics.should_fix
                        ? 0.5f
                        : 1.0f;

                glm_vec3_scale(mtv, scale, mtv);

                if (other->physics.should_fix) {
                    object_move(other, mtv);
                }

                if (object->physics.should_fix) {
                    object_move(object, mtv);
                }
            }
        }

        if (!found_collision) {
            object_move(object, velocity_scaled);
        }
    }
}
