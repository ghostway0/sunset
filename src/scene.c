#include <stdlib.h>

#include "sunset/scene.h"
#include "sunset/utils.h"

static bool should_split(struct oct_tree *tree, struct oct_node *node) {
    unused(tree);

    struct chunk *chunk = (struct chunk *)node->data;
    return chunk->num_objects > 5;
}

static void *split(struct oct_tree *, void *data, struct box bounds) {
    struct chunk *chunk = (struct chunk *)data;
    unused(bounds);
    unused(chunk);
    return NULL;
}

static void destroy_chunk(void *data) {
    struct chunk *chunk = (struct chunk *)data;
    free(chunk->objects);
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

struct chunk *get_chunk_for(struct scene const *scene, vec3 position) {
    return (struct chunk *)oct_tree_query(&scene->oct_tree, position);
}

void scene_destroy(struct scene *scene) {
    oct_tree_destroy(&scene->oct_tree);
}
