#pragma once

#include <stddef.h>

#include <cglm/types.h>

#include "sunset/vector.h"

struct byte_stream;

struct face_element {
    uint32_t vertex_index;
    uint32_t texcoord_index;
    uint32_t normal_index;
};

struct obj_model {
    vector(vec3) vertices;
    vector(vec3) normals;
    vector(vec2) texcoords;
    vector(vector(struct face_element)) faces;
    char *material_lib;
    char *object_name;
};

void obj_model_destroy(struct obj_model *model);

/// assumptions:
/// - the OBJ file represents a single object/mesh.
/// - the object has a single material applied to it.
/// - the file adheres to Blender's typical OBJ export conventions.
int obj_model_parse(struct byte_stream *stream, struct obj_model *model_out);
