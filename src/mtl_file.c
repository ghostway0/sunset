#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/types.h>
#include <log.h>

#include "sunset/errors.h"
#include "sunset/io.h"
#include "sunset/vector.h"

#include "sunset/mtl_file.h"

static void material_init(Material *material_out) {
    memset(material_out, 0, sizeof(Material));
    material_out->d = 1.0f;
}

int mtl_file_parse(Reader *reader, vector(Material) * materials_out) {
    int retval = 0;
    size_t line_number = 1;

    vector(uint8_t) line_buffer;
    vector_init(line_buffer);

    Material *current_material = NULL;

    while (true) {
        vector_clear(line_buffer);

        if (reader_read_until(reader, '\n', &line_buffer) == 0) {
            break;
        }

        line_buffer[vector_size(line_buffer) - 1] = '\0';
        char *line = (char *)line_buffer;

        if (line[0] == '#' || line[0] == '\0') {
            line_number++;
            continue;
        }

        if (strncmp(line, "newmtl ", 7) == 0) {
            size_t material_count = vector_size(*materials_out);
            vector_resize(*materials_out, material_count + 1);
            current_material = *materials_out + material_count;
            material_init(current_material);
            current_material->name = strdup(line + 7);
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

    return retval;
}

void material_destroy(Material *mtl) {
    free(mtl->name);
    free(mtl->map_kd);
    free(mtl->map_ke);
}
