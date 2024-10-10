// #define _GNU_SOURCE
#include <stdlib.h>

#include "sunset/scene.h"
#include "sunset/utils.h"

struct box from_rect(struct rect rect) {
    return (struct box){{rect.x, rect.y, 0.0f},
            {rect.x + rect.width, rect.y + rect.height, 0.0f}};
}

bool position_within_rect(vec3 position, struct rect rect) {
    return position[0] >= rect.x && position[0] <= rect.x + rect.width
            && position[1] >= rect.y && position[1] <= rect.y + rect.height;
}

bool position_within_box(vec3 position, struct box box) {
    return position[0] >= box.min[0] && position[0] <= box.max[0]
            && position[1] >= box.min[1] && position[1] <= box.max[1]
            && position[2] >= box.min[2] && position[2] <= box.max[2];
}

struct chunk *get_chunk_for(struct scene const *scene, vec3 position) {
    for (size_t i = 0; i < scene->num_chunks; ++i) {
        struct chunk *chunk = scene->chunks + i;
        if (position_within_rect(position, chunk->bounds)) {
            return chunk;
        }
    }

    return NULL;
}

static float rect_distance_to_camera(float *camera_position, struct rect rect) {
    vec3 center = {rect.x + (float)rect.width / 2,
            rect.y + (float)rect.height / 2,
            0.0f};
    return glm_vec3_distance(camera_position, center);
}

#ifdef __APPLE__
static int compare_chunks(
        void *camera_position_ptr, void const *a, void const *b) {
#else
int compare_chunks(void const *a, void const *b, void *camera_position_ptr) {
#endif
    const struct chunk *chunk_a = (const struct chunk *)a;
    const struct chunk *chunk_b = (const struct chunk *)b;
    float *camera_position = (float *)camera_position_ptr;

    float distance_a =
            rect_distance_to_camera(camera_position, chunk_a->bounds);
    float distance_b =
            rect_distance_to_camera(camera_position, chunk_b->bounds);

    if (distance_a < distance_b)
        return -1;
    if (distance_a > distance_b)
        return 1;
    return 0;
}

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks) {
    sunset_qsort(chunks,
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

    sunset_qsort(scene->chunks,
            scene->num_chunks,
            sizeof(struct chunk),
            scene->camera->position,
            compare_chunks);
}

bool object_within_frustum(const struct object *object, struct camera *camera) {
    struct box box = object->bounding_box;
    vec3 corners[8] = {
            {box.min[0], box.min[1], box.min[2]},
            {box.min[0], box.min[1], box.max[2]},
            {box.min[0], box.max[1], box.min[2]},
            {box.min[0], box.max[1], box.max[2]},
            {box.max[0], box.min[1], box.min[2]},
            {box.max[0], box.min[1], box.max[2]},
            {box.max[0], box.max[1], box.min[2]},
            {box.max[0], box.max[1], box.max[2]},
    };

    for (size_t i = 0; i < 8; ++i) {
        if (camera_point_in_frustum(camera, corners[i])) {
            return true;
        }
    }

    return false;
}
