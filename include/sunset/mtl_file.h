#ifndef SUNSET_MTL_FILE_H
#define SUNSET_MTL_FILE_H

#include <cglm/types.h>

#include "sunset/vector.h"

typedef struct Reader Reader;

typedef struct Material {
    vec3 kd; // diffuse color
    vec3 ks; // specular color
    float ns; // specular exponent
    float d; // dissolve (transparency)
    char *map_kd; // diffuse texture map
    char *map_ke; // emission texture map
    char *name;
} Material;

int mtl_file_parse(Reader *reader, vector(Material) *materials_out);

#endif // SUNSET_MTL_FILE_H
