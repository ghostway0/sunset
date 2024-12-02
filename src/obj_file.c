#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/types.h>
#include <log.h>

#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

struct face_element {
    size_t vertex_index;
    size_t texcoord_index;
    size_t normal_index;
};

struct obj_model {
    vector(vec3) vertices;
    vector(vec3) normals;
    vector(vec2) texcoords;
    vector(vector(struct face_element)) faces;
    char *material_lib;
    char *object_name;
};

static int parse_face_element(
        char *str, size_t vertex_count, struct face_element *face_out) {
    char *token;

    enum {
        FACE_PARSING_VERTEX,
        FACE_PARSING_TEXCOORD,
        FACE_PARSING_NORMAL,
    } stage = FACE_PARSING_VERTEX;

    while ((token = strsep(&str, "/")) != NULL) {
        size_t len = strlen(token);

        if (len == 0) {
            // TODO: empty element; might have a bug here
            continue;
        }

        errno = 0;
        int64_t value = strtol(token, NULL, 10);

        // NOTE: does not support negative indices
        if (errno == ERANGE || value <= 0) {
            log_error("cannot take this at face value.");
            return ERROR_INVALID_FORMAT;
        }

        size_t index = (size_t)value - 1;

        if (index >= vertex_count) {
            log_error("face index larger than vertex count");
            return ERROR_INVALID_FORMAT;
        }

        switch (stage) {
            case FACE_PARSING_VERTEX:
                face_out->vertex_index = index;
                stage = FACE_PARSING_TEXCOORD;
                break;
            case FACE_PARSING_TEXCOORD:
                face_out->texcoord_index = index;
                stage = FACE_PARSING_NORMAL;
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

static int parse_face(
        char *str, size_t vertex_count, vector(struct face_element) face_out) {
    int err = 0;

    char *token;

    while ((token = strsep(&str, " ")) != NULL) {
        struct face_element current_face;

        if ((err = parse_face_element(token, vertex_count, &current_face))) {
            return err;
        }

        vector_append(face_out, current_face);
    }

    return 0;
}

void obj_model_destroy(struct obj_model *model) {
    for (size_t i = 0; i < vector_size(model->faces); i++) {
        vector_destroy(model->faces[i]);
    }
    vector_destroy(model->faces);

    vector_destroy(model->texcoords);
    vector_destroy(model->normals);
    vector_destroy(model->vertices);
    free(model->material_lib);
    free(model->object_name);
}

void obj_model_init(struct obj_model *model_out) {
    vector_init(model_out->vertices);
    vector_init(model_out->normals);
    vector_init(model_out->texcoords);
    vector_init(model_out->faces);
}

/// assumptions:
/// - the OBJ file represents a single object/mesh.
/// - the object has a single material applied to it.
/// - the file adheres to Blender's typical OBJ export conventions.
int parse_obj(struct byte_stream *stream, struct obj_model *model_out) {
    int retval = 0;
    size_t line_number = 1;

    vector(uint8_t) line_buffer;
    vector_init(line_buffer);

    obj_model_init(model_out);

    while (!byte_stream_is_eof(stream)) {
        vector_clear(line_buffer);

        byte_stream_read_until(stream, '\n', line_buffer);
        byte_stream_skip(stream, 1);
        vector_append(line_buffer, '\0');
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
            vector_append_copy(model_out->vertices, vertex);
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
            vector_append_copy(model_out->normals, normal);
        } else if (strncmp(line, "vt ", 3) == 0) {
            vec2 texcoord;
            retval = sscanf(line + 3, "%f %f", &texcoord[0], &texcoord[1]) == 2
                    ? 0
                    : ERROR_INVALID_FORMAT;
            vector_append_copy(model_out->texcoords, texcoord);
        } else if (strncmp(line, "f ", 2) == 0) {
            size_t vertex_count = vector_size(model_out->vertices);
            size_t face_count = vector_size(model_out->faces);

            vector_resize(model_out->faces, face_count + 1);
            vector_init(model_out->faces[face_count]);

            retval = parse_face(
                    line + 2, vertex_count, model_out->faces[face_count]);
        } else if (strncmp(line, "mtllib ", 7) == 0) {
            assert(model_out->material_lib == NULL);
            model_out->material_lib = sunset_strdup(line + 7);
        } else if (strncmp(line, "g ", 2) == 0) {
            assert(model_out->object_name == NULL);
            model_out->object_name = sunset_strdup(line + 2);
        } else {
            log_warn("unsupported element at line %zu: %s", line_number, line);
        }

        if (retval) {
            log_error("parsing at line %zu: %s\n", line_number, line);
            break;
        }

        line_number++;
    }

    vector_destroy(line_buffer);
    if (retval) {
        obj_model_destroy(model_out);
    }

    return retval;
}
