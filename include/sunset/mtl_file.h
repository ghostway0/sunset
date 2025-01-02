#ifndef SUNSET_MTL_FILE_H
#define SUNSET_MTL_FILE_H

#include <cglm/types.h>

#include "sunset/vector.h"

typedef struct Reader Reader;

struct mtl_material {
    char *name;
    vec3 kd; // diffuse color
    vec3 ks; // specular color
    float ns; // specular exponent
    float d; // dissolve (transparency)
    char *map_kd; // diffuse texture map
    char *map_ke; // emission texture map
};

struct mtl_file {
    vector(struct mtl_material) materials;
};

int mtl_file_parse(Reader *reader, struct mtl_file *mtl_out);
void mtl_file_destroy(struct mtl_file *mtl);

#endif // SUNSET_MTL_FILE_H
