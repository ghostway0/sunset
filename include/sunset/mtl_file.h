#ifndef SUNSET_MTL_FILE_H
#define SUNSET_MTL_FILE_H

#include <cglm/types.h>

#include "sunset/vector.h"

struct mtl_material {
    char *name;
    vec3 kd; // Diffuse color
    vec3 ks; // Specular color
    float ns; // Specular exponent
    float d; // Dissolve (transparency)
    char *map_kd; // Diffuse texture map
    char *map_ke; // Emission texture map
};

struct mtl_file {
    vector(struct mtl_material) materials;
};

int mtl_file_parse(struct byte_stream *stream, struct mtl_file *mtl_out);
void mtl_file_destroy(struct mtl_file *mtl);

#endif // SUNSET_MTL_FILE_H
