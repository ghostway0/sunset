#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cglm/types.h>
#include <cglm/vec3.h>

#include "sunset/camera.h"
#include "sunset/geometry.h"

struct effect {
    void *program;
    struct texture *textures;
    size_t num_textures;
    struct uniform *uniforms;
    size_t num_uniforms;
};

struct object {
    struct box bounding_box;
    // TODO: can there be multiple meshes?
    struct mesh *meshes;
    size_t num_meshes;
    vec3 position;
    vec3 rotation;
    float scale;
    struct effect *effects;
    size_t num_effects;

    struct object *parent;
    struct object **children;
    size_t num_children;
};

bool object_within_frustum(struct object const *object, struct camera *camera);

enum light_type {
    LIGHT_DIRECTIONAL,
    LIGHT_SPOTLIGHT,
};

struct light {
    vec3 position;
    vec3 color;
    float intensity;
};

struct chunk {
    struct rect bounds;
    struct object *objects;
    size_t num_objects;
    struct light *lights;
    size_t num_lights;
    size_t id;
};

struct scene {
    // TODO: use the quadtree
    struct chunk *chunks;
    size_t num_chunks;
    struct camera *camera;
    struct mesh skybox;
    struct effect *effects;
    size_t num_effects;
};

bool position_within_box(vec3 position, struct box box);

struct chunk *get_chunk_for(struct scene const *scene, vec3 position);

void scene_load_chunks(
        struct scene *scene, struct chunk *chunks, size_t num_chunks);

void scene_move_camera(struct scene *scene, vec3 direction);
