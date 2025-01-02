#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/types.h>
#include <log.h>

#include "sunset/errors.h"
#include "sunset/io.h"
#include "sunset/mtl_file.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

static void mtl_material_init(struct mtl_material *material) {
    memset(material, 0, sizeof(struct mtl_material));
    material->d = 1.0f;
}

int mtl_file_parse(Reader *reader, struct mtl_file *mtl_out) {
    int retval = 0;
    size_t line_number = 1;

    vector_init(mtl_out->materials);

    vector(uint8_t) line_buffer;
    vector_init(line_buffer);

    struct mtl_material *current_material = NULL;

    while (true) {
        vector_clear(line_buffer);

        if (reader_read_until(reader, '\n', &line_buffer) == 0) {
            return 0;
        }

        line_buffer[vector_size(line_buffer) - 1] = '\0';
        char *line = (char *)line_buffer;

        if (line[0] == '#' || line[0] == '\0') {
            line_number++;
            continue;
        }

        if (strncmp(line, "newmtl ", 7) == 0) {
            size_t material_count = vector_size(mtl_out->materials);
            vector_resize(mtl_out->materials, material_count + 1);
            current_material = &mtl_out->materials[material_count];
            mtl_material_init(current_material);
            current_material->name = sunset_strdup(line + 7);
        } else if (strncmp(line, "Kd ", 3) == 0) {
            assert(current_material != NULL);
            retval = sscanf(line + 3,
                             "%f %f %f",
                             &current_material->kd[0],
                             &current_material->kd[1],
                             &current_material->kd[2])
                            == 3
                    ? 0
                    : ERROR_INVALID_FORMAT;
        } else if (strncmp(line, "Ks ", 3) == 0) {
            assert(current_material != NULL);
            retval = sscanf(line + 3,
                             "%f %f %f",
                             &current_material->ks[0],
                             &current_material->ks[1],
                             &current_material->ks[2])
                            == 3
                    ? 0
                    : ERROR_INVALID_FORMAT;
        } else if (strncmp(line, "Ns ", 3) == 0) {
            assert(current_material != NULL);
            retval = sscanf(line + 3, "%f", &current_material->ns) == 1
                    ? 0
                    : ERROR_INVALID_FORMAT;
        } else if (strncmp(line, "d ", 2) == 0) {
            assert(current_material != NULL);
            retval = sscanf(line + 2, "%f", &current_material->d) == 1
                    ? 0
                    : ERROR_INVALID_FORMAT;
        } else if (strncmp(line, "map_Kd ", 7) == 0) {
            assert(current_material != NULL);
            current_material->map_kd = sunset_strdup(line + 7);
        } else if (strncmp(line, "map_Ke ", 7) == 0) {
            assert(current_material != NULL);
            current_material->map_ke = sunset_strdup(line + 7);
        } else {
            log_warn("unsupported element at line %zu: %s",
                    line_number,
                    line);
        }

        if (retval) {
            log_error("parsing at line %zu: %s\n", line_number, line);
            break;
        }

        line_number++;
    }

    vector_destroy(line_buffer);
    if (retval) {
        mtl_file_destroy(mtl_out);
    }

    return retval;
}

void mtl_file_destroy(struct mtl_file *mtl) {
    for (size_t i = 0; i < vector_size(mtl->materials); i++) {
        free(mtl->materials[i].name);
        free(mtl->materials[i].map_kd);
        free(mtl->materials[i].map_ke);
    }
    vector_destroy(mtl->materials);
}
