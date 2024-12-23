#include <stddef.h>
#include <stdint.h>

#include <cglm/vec3.h>

#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/map.h"
#include "sunset/math.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

#include "sunset/physics.h"

#define VELOCITY_EPSILON 0.1

enum order compare_collisions(void const *a, void const *b) {
    const struct collision_pair *col_a = (const struct collision_pair *)a;
    const struct collision_pair *col_b = (const struct collision_pair *)b;

    // FIXME:
    // this is quite an annoying way of ensuring map is consistent between
    // frames.
    // 1. a has priority for no reason, and
    // 2. only technically not UB because of how scene is structured

    if (col_a->a < col_b->a) {
        return ORDER_LESS_THAN;
    }

    if (col_a->a > col_b->a) {
        return ORDER_GREATER_THAN;
    }

    if (col_a->b < col_b->b) {
        return ORDER_LESS_THAN;
    }

    if (col_a->b > col_b->b) {
        return ORDER_GREATER_THAN;
    }

    return ORDER_EQUAL;
}

static void aabb_collision_normal(
        struct aabb aabb, vec3 direction, vec3 normal_out) {
    vec3 center, inv_dir;
    aabb_get_center(&aabb, center);

    inv_dir[0] = 1.0f / direction[0];
    inv_dir[1] = 1.0f / direction[1];
    inv_dir[2] = 1.0f / direction[2];

    float tmin = (aabb.min[0] - center[0]) * inv_dir[0];
    float tmax = (aabb.max[0] - center[0]) * inv_dir[0];

    size_t axis = 0;
    float tmin_temp = (aabb.min[1] - center[1]) * inv_dir[1];
    float tmax_temp = (aabb.max[1] - center[1]) * inv_dir[1];

    if (tmin_temp > tmin) {
        tmin = tmin_temp;
        axis = 1;
    }
    if (tmax_temp < tmax) {
        tmax = tmax_temp;
        axis = 1;
    }

    tmin_temp = (aabb.min[2] - center[2]) * inv_dir[2];
    tmax_temp = (aabb.max[2] - center[2]) * inv_dir[2];

    if (tmin_temp > tmin) {
        tmin = tmin_temp;
        axis = 2;
    }
    if (tmax_temp < tmax) {
        tmax = tmax_temp;
        axis = 2;
    }

    glm_vec3_zero(normal_out);
    normal_out[axis] = (tmin < tmax) ? 1.0 : -1.0;
}

static void calculate_mtv(struct aabb a, struct aabb b, vec3 mtv_out) {
    vec3 overlap_min, overlap_max;

    assert(aabb_collide(&a, &b));

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

void physics_init(struct physics *physics) {
    vector_init(physics->objects);
    vector_init(physics->constraints);
    vector_init(physics->collision_pairs);
}

// NOTE: this is not the most accurate. you can see that when two objects
// are constrained they are moving at slightly different speeds than if
// they were not constrained together.
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

static void calculate_collision_normal(
        struct object *a, struct object *b, vec3 collision_normal_out) {
    if (glm_vec3_norm2(a->physics.velocity)
            > glm_vec3_norm2(b->physics.velocity)) {
        aabb_collision_normal(
                b->bounding_box, a->physics.velocity, collision_normal_out);
    } else {
        aabb_collision_normal(
                a->bounding_box, b->physics.velocity, collision_normal_out);
    }

    glm_vec3_normalize(collision_normal_out);
}

static void apply_collision_impulse(struct object *a, struct object *b) {
    struct physics_object *a_attr = &a->physics;
    struct physics_object *b_attr = &b->physics;

    assert(a_attr->type == PHYSICS_OBJECT_REGULAR
            || b_attr->type == PHYSICS_OBJECT_REGULAR);

    struct physics_material combined_material =
            combine_materials(a_attr->material, b_attr->material);

    vec3 collision_normal;
    calculate_collision_normal(a, b, collision_normal);

    if (a_attr->type == PHYSICS_OBJECT_REGULAR
            && b_attr->type == PHYSICS_OBJECT_REGULAR) {
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

    if (a_attr->type == PHYSICS_OBJECT_REGULAR) {
        // b's mass is infinite
        vec3 v1_normal, v1_tangent;

        glm_vec3_proj(a_attr->velocity, collision_normal, v1_normal);
        glm_vec3_sub(a_attr->velocity, v1_normal, v1_tangent);

        glm_vec3_scale(collision_normal,
                -glm_vec3_norm(v1_normal) * combined_material.restitution,
                v1_normal);

        object_set_velocity(a, v1_normal);
        object_add_velocity(a, v1_tangent);
    } else if (b_attr->type == PHYSICS_OBJECT_REGULAR) {
        // a's mass is infinite
        vec3 v2_normal, v2_tangent;

        glm_vec3_proj(b_attr->velocity, collision_normal, v2_normal);
        glm_vec3_sub(b_attr->velocity, v2_normal, v2_tangent);

        glm_vec3_scale(collision_normal,
                -glm_vec3_norm(v2_normal) * combined_material.restitution,
                v2_normal);

        object_set_velocity(b, v2_normal);
        object_add_velocity(b, v2_tangent);
    }
}

static void handle_object_collision(struct object *object,
        struct object *other,
        vec3 direction,
        struct event_queue *event_queue,
        vec3 direction_out) {
    glm_vec3_copy(direction, direction_out);

    struct event event = {
            .type_id = SYSTEM_EVENT_COLLISION,
            .collision =
                    {
                            .a = object,
                            .b = other,
                    },
    };

    if (!is_zero_vector(vec3, object->physics.velocity, VELOCITY_EPSILON)
            || !is_zero_vector(
                    vec3, other->physics.velocity, VELOCITY_EPSILON)) {
        glm_vec3_copy(object->physics.velocity, event.collision.a_velocity);
        glm_vec3_copy(other->physics.velocity, event.collision.b_velocity);

        event_queue_push(event_queue, event);
    }

    if (one_matches(PHYSICS_OBJECT_COLLIDER,
                object->physics.type,
                other->physics.type)
            || all_match(PHYSICS_OBJECT_INFINITE,
                    object->physics.type,
                    other->physics.type)) {
        return;
    }

    apply_collision_impulse(object, other);

    vec3 collision_normal;
    calculate_collision_normal(object, other, collision_normal);

    vec3 normal_direction;
    glm_vec3_proj(direction, collision_normal, normal_direction);
    glm_vec3_sub(direction, normal_direction, direction_out);
}

static void resolve_object_overlap(
        struct scene *scene, struct object *a, struct object *b) {
    vec3 mtv;
    calculate_mtv(a->bounding_box, b->bounding_box, mtv);

    float scale = a->physics.type == PHYSICS_OBJECT_REGULAR
                    && b->physics.type == PHYSICS_OBJECT_REGULAR
            ? 0.5f
            : 1.0f;

    glm_vec3_scale(mtv, scale, mtv);

    if (a->physics.type == PHYSICS_OBJECT_REGULAR) {
        scene_move_object_with_parent(scene, b, mtv);
    }

    if (a->physics.type == PHYSICS_OBJECT_REGULAR) {
        scene_move_object_with_parent(scene, a, mtv);
    }
}

static void generate_collision_event(struct event_queue *event_queue,
        struct collision_pair collision,
        enum collision_type collision_type) {
    struct physics_object a_attr = collision.a->physics;
    struct physics_object b_attr = collision.b->physics;

    // currently these are the limitations
    assert(!all_match(PHYSICS_OBJECT_COLLIDER, a_attr.type, b_attr.type)
            && "collider objects shouldn't collide with each other.");
    // NOTE: does this one make sense?
    assert(!all_match(PHYSICS_OBJECT_INFINITE, a_attr.type, b_attr.type)
            && "infinite-massed objects shouldn't collide with each other.");

    struct event event = {
            .type_id = SYSTEM_EVENT_COLLISION,
            .collision = {.a = collision.a,
                    .b = collision.b,
                    .type = collision_type},
    };

    // if event is a collider collision, b is the collider.
    swap_if(a_attr.type == PHYSICS_OBJECT_COLLIDER,
            event.collision.a,
            event.collision.b);

    event_queue_push(event_queue, event);
}

static bool physics_move_object_with_collisions(struct scene *scene,
        struct object *object,
        vec3 direction,
        struct event_queue *event_queue,
        map(struct collision_pair) new_collisions_out) {
    bool found_collision = false;

    vec3 moved;
    glm_vec3_add(object->transform.position, direction, moved);

    struct aabb path_box = object->bounding_box;
    aabb_extend_to(&path_box, moved);

    struct chunk *chunk = scene_get_chunk_for(scene, moved);

    vec3 new_direction;
    glm_vec3_copy(direction, new_direction);

    for (size_t j = 0; j < vector_size(chunk->objects); j++) {
        struct object *other = chunk->objects[j];

        if (object == other) {
            continue;
        }

        if (aabb_collide(&path_box, &other->bounding_box)) {
            handle_object_collision(
                    object, other, direction, event_queue, new_direction);

            bool collider_event = one_matches(PHYSICS_OBJECT_COLLIDER,
                    object->physics.type,
                    other->physics.type);

            if (new_collisions_out && collider_event) {
                struct collision_pair collision = {
                        .a = min(object, other),
                        .b = max(object, other),
                };

                map_insert(new_collisions_out, collision, compare_collisions);
            }

            found_collision = true;
        }

        if (aabb_collide(&object->bounding_box, &other->bounding_box)) {
            resolve_object_overlap(scene, object, other);
        }
    }

    scene_move_object_with_parent(scene, object, new_direction);

    return found_collision;
}

static void generate_collider_events(struct physics const *physics,
        struct event_queue *event_queue,
        map(const struct collision_pair) new_collisions) {
    size_t old_index = 0;
    size_t new_index = 0;

    while (old_index < vector_size(physics->collision_pairs)
            && new_index < vector_size(new_collisions)) {
        struct collision_pair old_collision =
                physics->collision_pairs[old_index];
        struct collision_pair new_collision = new_collisions[new_index];

        enum order cmp = compare_collisions(&old_collision, &new_collision);

        if (cmp == ORDER_LESS_THAN) {
            generate_collision_event(
                    event_queue, old_collision, COLLISION_EXIT_COLLIDER);
            old_index++;
        } else if (cmp == ORDER_GREATER_THAN) {
            generate_collision_event(
                    event_queue, new_collision, COLLISION_ENTER_COLLIDER);
            new_index++;
        } else {
            old_index++;
            new_index++;
        }
    }

    // remaining entries are exits
    while (old_index < vector_size(physics->collision_pairs)) {
        struct collision_pair old_collision =
                physics->collision_pairs[old_index];

        generate_collision_event(
                event_queue, old_collision, COLLISION_EXIT_COLLIDER);
        old_index++;
    }

    // remaining entries are entries
    while (new_index < vector_size(new_collisions)) {
        struct collision_pair new_collision = new_collisions[new_index];
        generate_collision_event(
                event_queue, new_collision, COLLISION_ENTER_COLLIDER);

        new_index++;
    }
}

void physics_destroy(struct physics *physics) {
    vector_destroy(physics->objects);
    vector_destroy(physics->constraints);
    vector_destroy(physics->collision_pairs);
}

void physics_add_object(struct physics *physics, struct object *object) {
    vector_append(physics->objects, object);

    if (object->physics.type == PHYSICS_OBJECT_REGULAR) {
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

bool physics_move_object(struct scene *scene,
        struct object *object,
        vec3 direction,
        struct event_queue *event_queue) {
    return physics_move_object_with_collisions(
            scene, object, direction, event_queue, NULL);
}

void physics_step(struct physics *physics,
        struct scene *scene,
        struct event_queue *event_queue,
        float dt) {
    apply_constraint_forces(physics, dt);

    map(struct collision_pair) new_collisions;
    map_init(new_collisions);

    for (size_t i = 0; i < vector_size(physics->objects); i++) {
        struct object *object = physics->objects[i];

        object_add_velocity(object, object->physics.acceleration);

        vec3 velocity_scaled;
        glm_vec3_scale(object->physics.velocity, dt, velocity_scaled);

        physics_move_object_with_collisions(
                scene, object, velocity_scaled, event_queue, new_collisions);
    }

    generate_collider_events(physics, event_queue, new_collisions);

    vector_destroy(physics->collision_pairs);
    physics->collision_pairs = new_collisions;
}

// TODO: merge physics_step and physics_callback
void physics_callback(struct engine_context *engine_context,
        void *physics,
        struct event /* engine tick */) {
    physics_step(physics,
            engine_context->scene,
            &engine_context->event_queue,
            engine_context->dt);
}
