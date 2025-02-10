#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/types.h>
#include <log.h>

#include "sunset/errors.h"
#include "sunset/geometry.h"
#include "sunset/io.h"
#include "sunset/obj_file.h"
#include "sunset/vector.h"

void model_init(Model *model_out) {
    vector_init(model_out->vertices);
    vector_init(model_out->normals);
    vector_init(model_out->texcoords);
    vector_init(model_out->faces);

    model_out->object_name = NULL;
    model_out->material_lib = NULL;
}

static int parse_face_element(
        char *str, size_t vertex_count, FaceElement *face_out) {
    char *token;

    memset(face_out, 0xFFFFFFFF, sizeof(FaceElement));

    enum {
        FACE_PARSING_INITIAL,
        FACE_PARSING_VERTEX,
        FACE_PARSING_TEXCOORD,
        FACE_PARSING_NORMAL,
        FACE_PARSING_INVALID,
    } stage = FACE_PARSING_INITIAL;

    while ((token = strsep(&str, "/")) != NULL) {
        stage++;

        size_t index = 0;
        size_t len = strlen(token);

        if (len == 0) {
            if (stage != FACE_PARSING_TEXCOORD) {
                return ERROR_INVALID_FORMAT;
            }

            continue;
        }

        errno = 0;
        int64_t value = strtol(token, NULL, 10);

        if (errno != 0 || value > UINT32_MAX) {
            log_error("cannot take this at face value.");
            return ERROR_INVALID_FORMAT;
        }

        index = value > 0 ? (uint32_t)value - 1 : vertex_count - value;

        if (index >= vertex_count) {
            log_error("face index larger than vertex count (%zu/%zu)", index, vertex_count);
            return ERROR_INVALID_FORMAT;
        }

        switch (stage) {
            case FACE_PARSING_VERTEX:
                face_out->vertex_index = index;
                break;
            case FACE_PARSING_TEXCOORD:
                face_out->texcoord_index = index;
                break;
            case FACE_PARSING_NORMAL:
                face_out->normal_index = index;
                return 0;
            default:
                unreachable();
        }
    }

    return ERROR_INVALID_FORMAT;
}

static int parse_face(char const *str,
        size_t vertex_count,
        vector(FaceElement) face_out) {
    int retval = 0;
    char *duped_str = sunset_strdup(str);
    char *dup_original = duped_str;

    char *token;
    while ((token = strsep(&duped_str, " ")) != NULL) {
        FaceElement current_face;

        if ((retval = parse_face_element(
                     token, vertex_count, &current_face))) {
            break;
        }

        vector_append(face_out, current_face);
    }

    free(dup_original);
    return retval;
}

void obj_model_destroy(Model *model) {
    for (size_t i = 0; i < vector_size(model->faces); i++) {
        if (model->faces[i] != NULL) {
            vector_destroy(model->faces[i]);
        }
    }
    vector_destroy(model->faces);

    vector_destroy(model->texcoords);
    vector_destroy(model->normals);
    vector_destroy(model->vertices);
    free(model->material_lib);

    model->material_lib = NULL;
}

bool model_is_empty(Model const *model) {
    return vector_size(model->vertices) == 0
            && vector_size(model->normals) == 0
            && vector_size(model->texcoords) == 0
            && vector_size(model->faces) == 0 && model->object_name == NULL
            && model->material_lib == NULL;
}

/// assumptions:
/// - the OBJ file represents a single object/mesh.
/// - the object has a single material applied to it.
/// - the file adheres to Blender's typical OBJ export conventions.
int obj_model_parse(Reader *reader, vector(Model) * models_out) {
    int retval = 0;
    size_t line_number = 1;
    size_t initial_size = vector_size(*models_out);

    vector(uint8_t) line_buffer;
    vector_init(line_buffer);

    Model current_model;
    model_init(&current_model);

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

        if (strncmp(line, "v ", 2) == 0) {
            vec3 vertex;
            retval = sscanf(line + 2,
                             "%f %f %f",
                             &vertex[0],
                             &vertex[1],
                             &vertex[2])
                            == 3
                    ? 0
                    : ERROR_INVALID_FORMAT;
            vector_append_copy(current_model.vertices, vertex);
        } else if (strncmp(line, "vn ", 3) == 0) {
            vec3 normal;
            retval = sscanf(line + 3,
                             "%f %f %f",
                             &normal[0],
                             &normal[1],
                             &normal[2])
                            == 3
                    ? 0
                    : ERROR_INVALID_FORMAT;
            vector_append_copy(current_model.normals, normal);
        } else if (strncmp(line, "vt ", 3) == 0) {
            vec2 texcoord;
            retval = sscanf(line + 3, "%f %f", &texcoord[0], &texcoord[1])
                            == 2
                    ? 0
                    : ERROR_INVALID_FORMAT;
            vector_append_copy(current_model.texcoords, texcoord);
        } else if (strncmp(line, "f ", 2) == 0) {
            size_t vertex_count = vector_size(current_model.vertices);
            size_t face_count = vector_size(current_model.faces);

            vector_resize(current_model.faces, face_count + 1);
            vector_init(current_model.faces[face_count]);

            retval = parse_face(line + 2,
                    vertex_count,
                    current_model.faces[face_count]);
        } else if (strncmp(line, "g ", 2) == 0
                || strncmp(line, "o ", 2) == 0) {
            if (!model_is_empty(&current_model)) {
                vector_append(*models_out, current_model);
                model_init(&current_model);
            }

            current_model.object_name = strdup(line + 2);
        } else if (strncmp(line, "mtllib ", 7) == 0) {
            if (current_model.material_lib) {
                retval = ERROR_INVALID_FORMAT;
            }

            current_model.material_lib = sunset_strdup(line + 7);
        } else {
        }

        if (retval) {
            log_error("parsing at line %zu: %s\n", line_number, line);
            break;
        }

        line_number++;
    }

    if (!model_is_empty(&current_model)) {
        vector_append(*models_out, current_model);
    }

    if (retval) {
        for (size_t i = initial_size - 1; i < vector_size(models_out);
                i++) {
            obj_model_destroy(*models_out + i);
        }

        vector_resize(*models_out, initial_size);
    }

    vector_destroy(line_buffer);

    return retval;
}
