#pragma once

#include <assert.h>
#include <stdlib.h>

struct vector_metadata {
    size_t size;
    size_t capacity;
};

#define vector(T) T *

#define vector_metadata(v) ((struct vector_metadata *)(v) - 1)

#define vector_create(v, T)                                                    \
    ({                                                                         \
        struct vector_metadata *meta = (struct vector_metadata *)malloc(       \
                sizeof(struct vector_metadata) + sizeof(T) * 16);              \
        assert(meta);                                                          \
        meta->size = 0;                                                        \
        meta->capacity = 16;                                                   \
        v = (void *)(meta + 1);                                                \
    })

#define vector_free(v)                                                         \
    do {                                                                       \
        free(vector_metadata(v));                                              \
        v = NULL;                                                              \
    } while (0)

#define vector_size(v) (vector_metadata(v)->size)

#define vector_capacity(v) (vector_metadata(v)->capacity)

#define vector_clear(v) vector_metadata(v)->size = 0

#define vector_reserve(v, new_capacity)                                        \
    do {                                                                       \
        struct vector_metadata *meta = vector_metadata(v);                     \
        if (meta->capacity < new_capacity) {                                   \
            meta = (struct vector_metadata *)realloc(meta,                     \
                    sizeof(struct vector_metadata)                             \
                            + sizeof(*(v)) * new_capacity);                    \
            assert(meta);                                                      \
            meta->capacity = new_capacity;                                     \
            v = (void *)(meta + 1);                                            \
        }                                                                      \
    } while (0)

#define vector_append(v, value)                                                \
    do {                                                                       \
        struct vector_metadata *meta = vector_metadata(v);                     \
        if (meta->size == meta->capacity) {                                    \
            meta->capacity *= 2;                                               \
            meta = (struct vector_metadata *)realloc(meta,                     \
                    sizeof(struct vector_metadata)                             \
                            + sizeof(*(v)) * meta->capacity);                  \
            assert(meta);                                                      \
            v = (void *)(meta + 1);                                            \
        }                                                                      \
        (v)[meta->size++] = value;                                             \
    } while (0)

#define vector_append_multiple(v, data, size)                                  \
    do {                                                                       \
        struct vector_metadata *meta = vector_metadata(v);                     \
        if (meta->size + size > meta->capacity) {                              \
            meta->capacity = meta->size + size;                                \
            meta = (struct vector_metadata *)realloc(meta,                     \
                    sizeof(struct vector_metadata)                             \
                            + sizeof(*(v)) * meta->capacity);                  \
            assert(meta);                                                      \
            v = (void *)(meta + 1);                                            \
        }                                                                      \
        memcpy((v) + meta->size, data, size);                                  \
        meta->size += size;                                                    \
    } while (0)

#define vector_pop(v)                                                          \
    ({                                                                         \
        struct vector_metadata *_meta = vector_metadata(v);                    \
        assert(_meta->size > 0);                                               \
        v[--_meta->size];                                                      \
    })
