#include <stdlib.h>
#include <string.h>

#include "cglm/affine.h"
#include "cglm/types.h"
#include "sunset/commands.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

#include "sunset/backend.h"

static bool should_split(struct oct_tree *tree, struct oct_node *node) {
    unused(tree);

    struct chunk *chunk = (struct chunk *)node->data;
    return chunk->num_objects > 5;
}

static void *split(struct oct_tree *, void *data, struct box bounds) {
    struct chunk *chunk = (struct chunk *)data;
    struct chunk *new_chunk = malloc(sizeof(struct chunk));

    *new_chunk = *chunk;
    new_chunk->bounds = bounds;

    vector(struct object *) in_new_bounds;
    vector_init(in_new_bounds, struct object);

    for (size_t i = 0; i < chunk->num_objects; ++i) {
        struct object *object = chunk->objects[i];

        if (position_within_box(object->transform.position, bounds)) {
            vector_append(in_new_bounds, object);
        }
    }

    new_chunk->num_objects = vector_size(in_new_bounds);
    new_chunk->objects = in_new_bounds;

    vector_free(in_new_bounds);

    return new_chunk;
}

static void destroy_chunk(void *data) {
    struct chunk *chunk = (struct chunk *)data;
    free(chunk->objects);
    free(chunk);
}

void scene_init(struct camera camera,
        struct image skybox,
        struct effect *effects,
        size_t num_effects,
        struct box bounds,
        struct chunk *root_chunk,
        struct scene *scene_out) {
    scene_out->camera = camera;
    scene_out->skybox = skybox;
    scene_out->effects = effects;
    scene_out->num_effects = num_effects;

    oct_tree_create(DEFAULT_MAX_OCTREE_DEPTH,
            should_split,
            split,
            destroy_chunk,
            root_chunk,
            bounds,
            &scene_out->oct_tree);
}

struct chunk *scene_get_chunk_for(struct scene const *scene, vec3 position) {
    return (struct chunk *)oct_tree_query(&scene->oct_tree, position);
}

void scene_destroy(struct scene *scene) {
    oct_tree_destroy(&scene->oct_tree);
}

void object_move(struct object *object, vec3 direction) {
    glm_vec3_add(
            object->transform.position, direction, object->transform.position);

    glm_vec3_add(object->bounding_box.min, direction, object->bounding_box.min);
    glm_vec3_add(object->bounding_box.max, direction, object->bounding_box.max);

    if (object->parent != NULL) {
        object_move(object->parent, direction);
    }

    for (size_t i = 0; i < object->num_children; ++i) {
        object_move(object->children[i], direction);
    }
}

static void object_calculate_model_matrix(
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

    command_buffer_add_mesh(command_buffer, object->mesh_id, 0, model_matrix);

    // TODO: add textures and materials

    return 0;
}

int scene_render(struct scene *scene, struct render_context *render_context) {
    struct chunk *chunk =
            oct_tree_query(&scene->oct_tree, scene->camera.position);

    for (size_t i = 0; i < chunk->num_objects; ++i) {
        render_object(chunk->objects[i], &render_context->command_buffer);
    }

    return 0;
}
