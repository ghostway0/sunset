#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include "sunset/camera.h"
#include "sunset/geometry.h"
#include "sunset/octree.h"

struct transform {
    vec3 position;
    vec3 rotation;
    float scale;
};

struct object {
    struct box bounding_box;
    struct transform transform;
    struct mesh *meshes;
    size_t num_meshes;
    struct texture *textures;
    size_t num_textures;
    struct material *materials;
    size_t num_materials;

    struct object *parent;
    struct object *children;
    size_t num_children;
};

enum light_type {
    LIGHT_DIRECTIONAL,
    LIGHT_SPOTLIGHT,
    LIGHT_POINT,
};

struct light {
    enum light_type type;
    vec3 position;
    vec3 color;
    float intensity;
};

struct chunk {
    struct box bounds;
    struct object *objects;
    size_t num_objects;
    struct light *lights;
    size_t num_lights;
    size_t id;
};

struct scene {
    struct oct_tree oct_tree;
    struct camera camera;
    struct image skybox;
    struct effect *effects;
    size_t num_effects;
};

void scene_init(struct camera camera,
        struct image skybox,
        struct effect *effects,
        size_t num_effects,
        struct box bounds,
        struct chunk *root_chunk,
        struct scene *scene_out);

bool position_within_box(vec3 position, struct box box);

struct chunk *get_chunk_for(struct scene const *scene, vec3 position);

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks);

void scene_move_camera(struct scene *scene, vec3 direction);

void scene_destroy(struct scene *scene);
