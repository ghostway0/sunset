#ifndef SUNSET_MTL_FILE_H
#define SUNSET_MTL_FILE_H

#include <cglm/types.h>

#include "sunset/vector.h"
#include "sunset/geometry.h"

typedef struct Reader Reader;

int mtl_file_parse(Reader *reader, vector(Material) *materials_out);

#endif // SUNSET_MTL_FILE_H
