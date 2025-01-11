#include <stdlib.h>
#include <string.h>

#include <cglm/affine.h>
#include <cglm/types.h>

#include "internal/utils.h"
#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/map.h"
#include "sunset/octree.h"
#include "sunset/vector.h"

#include "sunset/scene.h"

static bool should_split(struct octree *tree, struct octree_node *node) {
    unused(tree);

    struct chunk *chunk = (struct chunk *)node->data;
    return vector_size(chunk->objects) > 5;
}

static void *split(struct octree *, void *data, AABB bounds) {
    struct chunk *chunk = (struct chunk *)data;
    struct chunk *new_chunk = sunset_malloc(sizeof(struct chunk));

    *new_chunk = *chunk;
    new_chunk->bounds = bounds;

    vector_init(new_chunk->objects);

    for (size_t i = 0; i < vector_size(chunk->objects); ++i) {
        struct object *object = chunk->objects[i];

        if (position_within_aabb(object->transform.position, bounds)) {
            vector_append(new_chunk->objects, object);
        }
    }

    return new_chunk;
}

static void destroy_chunk(void *data) {
    struct chunk *chunk = (struct chunk *)data;
    free(chunk->objects);
    free(chunk);
}

static struct chunk *scene_get_mutable_chunk_for(
        struct scene *scene, vec3 position) {
    return (struct chunk *)octree_get_mutable(&scene->octree, position);
}

static void move_object_chunk(
        struct scene *scene, struct object *object, vec3 from, vec3 to) {
    struct chunk *old_chunk = scene_get_mutable_chunk_for(scene, from);
    struct chunk *new_chunk = scene_get_mutable_chunk_for(scene, to);

    map_remove(old_chunk->objects, object, compare_ptrs);
    map_insert(new_chunk->objects, object, compare_ptrs);
}

void scene_set_size(struct scene *scene, AABB new_bounds) {
    struct chunk *root_chunk = sunset_malloc(sizeof(struct chunk));

    root_chunk->bounds = new_bounds;
    root_chunk->id = 0;
    vector_init(root_chunk->objects);

    for (size_t i = 0; i < vector_size(scene->objects); i++) {
        vector_append(root_chunk->objects, &scene->objects[i]);
    }

    octree_create(DEFAULT_MAX_OCTREE_DEPTH,
            should_split,
            split,
            destroy_chunk,
            root_chunk,
            new_bounds,
            &scene->octree);
}

void scene_init(struct image skybox, AABB bounds, struct scene *scene_out) {
    vector_init(scene_out->cameras);
    vector_init(scene_out->objects);

    scene_out->skybox = skybox;

    scene_set_size(scene_out, bounds);
}

struct chunk *scene_get_chunk_for(
        struct scene const *scene, vec3 position) {
    return (struct chunk *)octree_query(&scene->octree, position);
}

void scene_destroy(struct scene *scene) {
    octree_destroy(&scene->octree);
    vector_destroy(scene->cameras);
}

void scene_move_object_with_parent(
        struct scene *scene, struct object *object, vec3 direction) {
    scene_move_object(scene, object, direction);

    if (object->parent != NULL) {
        scene_move_object(scene, object->parent, direction);
    }
}

void scene_move_object(
        struct scene *scene, struct object *object, vec3 direction) {
    vec3 new_position;

    glm_vec3_add(object->transform.position, direction, new_position);
    aabb_translate(&object->bounding_box, direction);

    move_object_chunk(
            scene, object, object->transform.position, new_position);

    glm_vec3_copy(new_position, object->transform.position);

    for (size_t i = 0; i < object->num_children; ++i) {
        scene_move_object(scene, object->children[i], direction);
    }

    if (object->move_callback != NULL) {
        object->move_callback(object, direction);
    }
}

void object_rotate(struct object *object, vec3 rotation) {
    glm_vec3_add(object->transform.rotation,
            rotation,
            object->transform.rotation);

    for (size_t i = 0; i < object->num_children; ++i) {
        object_rotate(object->children[i], rotation);
    }
}

static void object_set_velocity_relative(
        struct object *object, vec3 velocity, vec3 parent_velocity) {
    vec3 diff;
    glm_vec3_sub(velocity, parent_velocity, diff);

    glm_vec3_add(object->physics.velocity, diff, object->physics.velocity);

    for (size_t i = 0; i < object->num_children; ++i) {
        object_set_velocity_relative(
                object->children[i], velocity, object->physics.velocity);
    }
}

void object_set_velocity(struct object *object, vec3 velocity) {
    for (size_t i = 0; i < object->num_children; ++i) {
        object_set_velocity_relative(
                object->children[i], velocity, object->physics.velocity);
    }

    glm_vec3_copy(velocity, object->physics.velocity);
}

void object_scale_velocity(struct object *object, float factor) {
    glm_vec3_scale(
            object->physics.velocity, factor, object->physics.velocity);

    for (size_t i = 0; i < object->num_children; ++i) {
        object_scale_velocity(object->children[i], factor);
    }
}

void object_add_velocity(struct object *object, vec3 acceleration) {
    glm_vec3_add(object->physics.velocity,
            acceleration,
            object->physics.velocity);

    for (size_t i = 0; i < object->num_children; ++i) {
        object_add_velocity(object->children[i], acceleration);
    }
}

void object_rotate_velocity(struct object *object, float angle, vec3 axis) {
    glm_vec3_rotate(object->physics.velocity, angle, axis);

    for (size_t i = 0; i < object->num_children; ++i) {
        object_rotate_velocity(object->children[i], angle, axis);
    }
}

void object_calculate_model_matrix(
        struct object *object, mat4 model_matrix) {
    glm_mat4_identity(model_matrix);

    glm_translate(model_matrix, object->transform.position);

    glm_rotate(model_matrix,
            object->transform.rotation[0],
            (vec3){1.0f, 0.0f, 0.0f});
    glm_rotate(model_matrix,
            object->transform.rotation[1],
            (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(model_matrix,
            object->transform.rotation[2],
            (vec3){0.0f, 0.0f, 1.0f});
}

static int render_object(
        struct object *object, struct command_buffer *command_buffer) {
    mat4 model_matrix;
    object_calculate_model_matrix(object, model_matrix);

    command_buffer_add_mesh(command_buffer,
            object->mesh_id,
            object->texture_id,
            model_matrix);

    return 0;
}

int scene_render(
        struct scene *scene, struct render_context *render_context) {
    // FIXME: doesn't make much sense - still need to think that through
    for (size_t i = 0; i < vector_size(scene->cameras); i++) {
        struct camera *camera = &scene->cameras[i];

        struct chunk *chunk =
                octree_query(&scene->octree, camera->position);

        for (size_t i = 0; i < vector_size(chunk->objects); ++i) {
            AABB object_bounds = chunk->objects[i]->bounding_box;

            if (camera_box_within_frustum(camera, object_bounds)) {
                render_object(
                        chunk->objects[i], &render_context->command_buffer);
            }
        }

        backend_draw(render_context,
                &render_context->command_buffer,
                camera->view_matrix,
                camera->projection_matrix);
    }

    return 0;
}

void scene_move_camera(
        struct scene *scene, size_t camera_index, vec3 direction) {
    camera_move_absolute(&scene->cameras[camera_index], direction);
}

void scene_rotate_camera(struct scene *scene,
        size_t camera_index,
        float x_angle,
        float y_angle) {
    camera_rotate_scaled(&scene->cameras[camera_index], x_angle, y_angle);
}

void scene_add_object(struct scene *scene, struct object object) {
    vector_append(scene->objects, object);

    struct chunk *chunk =
            scene_get_mutable_chunk_for(scene, object.transform.position);
    map_insert_ptr(
            chunk->objects, vector_back(scene->objects), compare_ptrs);
}

int scene_load_config() {
    todo();
    return 0;
}
