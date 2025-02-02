#pragma once

#include <stddef.h>

#include <cglm/types.h>

#include "sunset/vector.h"

typedef struct Reader Reader;

typedef struct FaceElement {
    uint32_t vertex_index;
    uint32_t texcoord_index;
    uint32_t normal_index;
} FaceElement;

typedef struct Model {
    vector(vec3) vertices;
    vector(vec3) normals;
    vector(vec2) texcoords;
    vector(vector(FaceElement)) faces;
    char *material_lib;
    char *object_name;
} Model;

/// assumptions:
/// - the OBJ file represents a single object/mesh.
/// - the object has a single material applied to it.
/// - the file adheres to Blender's typical OBJ export conventions.
int obj_model_parse(Reader *reader, vector(Model) *models_out);
