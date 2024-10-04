#include "sunset/scene.h"

bool position_within_box(vec3 position, struct box box) {
    return position[0] >= box.min[0] && position[0] <= box.max[0]
            && position[1] >= box.min[1] && position[1] <= box.max[1]
            && position[2] >= box.min[2] && position[2] <= box.max[2];
}

struct chunk *get_chunk_for(struct scene const *scene, vec3 position) {
    for (size_t i = 0; i < scene->num_chunks; ++i) {
        struct chunk *chunk = scene->chunks + i;
        if (position_within_box(position, chunk->bounding_box)) {
            return chunk;
        }
    }

    return NULL;
}

static float distance_to_camera(vec3 camera_position, struct box bounding_box) {
    vec3 center;
    glm_vec3_add(bounding_box.min, bounding_box.max, center);
    glm_vec3_scale(center, 0.5f, center); // Get the center of the bounding box

    return glm_vec3_distance(camera_position, center);
}

#ifdef __APPLE__
static int compare_chunks(void *camera_position_ptr, void const *a, void const *b) {
#else
int compare_chunks(void const *a, void const *b, void *camera_position_ptr) {
#endif
    const struct chunk *chunk_a = (const struct chunk *)a;
    const struct chunk *chunk_b = (const struct chunk *)b;
    float *camera_position = (float *)camera_position_ptr;

    float distance_a =
            distance_to_camera(camera_position, chunk_a->bounding_box);
    float distance_b =
            distance_to_camera(camera_position, chunk_b->bounding_box);

    if (distance_a < distance_b)
        return -1;
    if (distance_a > distance_b)
        return 1;
    return 0;
}

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks) {
    qsort_r(chunks,
            num_chunks,
            sizeof(struct chunk),
            scene->camera->position,
            compare_chunks);

    scene->chunks = chunks;
    scene->num_chunks = num_chunks;
}

void scene_move_camera(struct scene *scene, vec3 direction) {
    camera_move(scene->camera, direction);

    // TODO: partial sort

    qsort_r(scene->chunks,
            scene->num_chunks,
            sizeof(struct chunk),
            scene->camera->position,
            compare_chunks);
}
