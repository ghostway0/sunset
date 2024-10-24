#include <stdlib.h>
#include <string.h>

#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

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
