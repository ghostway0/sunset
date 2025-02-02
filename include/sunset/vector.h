#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "internal/mem_utils.h"

struct vector_metadata {
    size_t size;
    size_t capacity;
};

#define vector(T) T *

#define _vector_metadata(v) ((struct vector_metadata *)(v) - 1)

#define vector_init(v)                                                     \
    do {                                                                   \
        struct vector_metadata *meta =                                     \
                (struct vector_metadata *)sunset_malloc(                   \
                        sizeof(struct vector_metadata) + sizeof(*v) * 16); \
        meta->size = 0;                                                    \
        meta->capacity = 16;                                               \
        v = (void *)(meta + 1);                                            \
    } while (0);

#define vector_destroy(v)                                                  \
    do {                                                                   \
        free(_vector_metadata(v));                                         \
        v = NULL;                                                          \
    } while (0)

#define vector_size(v) (_vector_metadata(v)->size)

#define vector_empty(v) (_vector_metadata(v)->size == 0)

#define vector_capacity(v) (_vector_metadata(v)->capacity)

#define vector_clear(v) _vector_metadata(v)->size = 0

#define vector_reserve(v, new_capacity)                                    \
    do {                                                                   \
        struct vector_metadata *meta = _vector_metadata(v);                \
        if (meta->capacity < new_capacity) {                               \
            meta = (struct vector_metadata *)sunset_realloc(meta,          \
                    sizeof(struct vector_metadata)                         \
                            + sizeof(*(v)) * new_capacity);                \
            assert(meta);                                                  \
            meta->capacity = new_capacity;                                 \
            v = (void *)(meta + 1);                                        \
        }                                                                  \
    } while (0)

#define PRAGMA_DISABLE_PTR_WARN                                            \
    _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-memaccess\"");

#define vector_append(v, ...)                                              \
    do {                                                                   \
        struct vector_metadata *meta = _vector_metadata(v);                \
        if (meta->size == meta->capacity) {                                \
            meta->capacity *= 2;                                           \
            _Pragma("GCC diagnostic push");                                \
            PRAGMA_DISABLE_PTR_WARN                                        \
            meta = (struct vector_metadata *)sunset_realloc(meta,          \
                    sizeof(struct vector_metadata)                         \
                            + meta->capacity * sizeof(*(v)));              \
            _Pragma("GCC diagnostic pop");                                 \
            v = (void *)(meta + 1);                                        \
        }                                                                  \
        (v)[meta->size++] = __VA_ARGS__;                                   \
    } while (0)

// use memcpy for types that are not trivially copyable
#define vector_append_copy(v, value)                                       \
    do {                                                                   \
        struct vector_metadata *meta = _vector_metadata(v);                \
        if (meta->size == meta->capacity) {                                \
            meta->capacity *= 2;                                           \
            _Pragma("GCC diagnostic push");                                \
            PRAGMA_DISABLE_PTR_WARN                                        \
            meta = (struct vector_metadata *)sunset_realloc(meta,          \
                    sizeof(struct vector_metadata)                         \
                            + sizeof(*(v)) * meta->capacity);              \
            _Pragma("GCC diagnostic pop");                                 \
            assert(meta);                                                  \
            v = (void *)(meta + 1);                                        \
        }                                                                  \
        memcpy(&(v)[meta->size++], &(value), sizeof(value));               \
    } while (0)

#define vector_append_multiple(v, data, size2)                             \
    do {                                                                   \
        struct vector_metadata *meta = _vector_metadata(v);                \
        size_t size = meta->size;                                          \
        vector_resize(v, size + size2);                                    \
        for (size_t i = 0; i < size2; i++) {                               \
            v[size + i] = data[i];                                         \
        }                                                                  \
    } while (0)

#define vector_pop_front(v)                                                \
    ({                                                                     \
        struct vector_metadata *_meta = _vector_metadata(v);               \
        assert(_meta->size > 0);                                           \
        typeof(*(v)) _front_value = (v)[0];                                \
        if (_meta->size > 0) {                                             \
            if (_meta->size > 1) {                                         \
                memmove(v,                                                 \
                        (char *)(v) + sizeof(*(v)),                        \
                        (_meta->size - 1) * sizeof(*(v)));                 \
            }                                                              \
            _meta->size--;                                                 \
        }                                                                  \
        _front_value;                                                      \
    })

#define vector_pop_back(v)                                                 \
    ({                                                                     \
        struct vector_metadata *_meta = _vector_metadata(v);               \
        assert(_meta->size > 0);                                           \
        v[--_meta->size];                                                  \
    })

#define vector_back(v)                                                     \
    ({                                                                     \
        struct vector_metadata *_meta = _vector_metadata(v);               \
        assert(_meta->size > 0);                                           \
        &v[_meta->size - 1];                                               \
    })

#define vector_resize(v, new_size)                                         \
    do {                                                                   \
        struct vector_metadata *meta = _vector_metadata(v);                \
        if (meta->capacity < new_size) {                                   \
            meta->capacity = new_size;                                     \
            meta = (struct vector_metadata *)sunset_realloc(meta,          \
                    sizeof(struct vector_metadata)                         \
                            + sizeof(*(v)) * meta->capacity);              \
            assert(meta);                                                  \
            v = (void *)(meta + 1);                                        \
        }                                                                  \
        if (new_size > meta->size) {                                       \
            memset(v + meta->size,                                         \
                    0,                                                     \
                    (new_size - meta->size) * sizeof(*v));                 \
        }                                                                  \
        meta->size = new_size;                                             \
    } while (0)

#define vector_remove_index(v, i)                                          \
    do {                                                                   \
        struct vector_metadata *_meta = _vector_metadata(v);               \
        assert(i < _meta->size);                                           \
        if (i < _meta->size - 1) {                                         \
            memmove((char *)(v + i),                                       \
                    (char *)(v + i + 1),                                   \
                    (_meta->size - i - 1) * sizeof(*(v)));                 \
        }                                                                  \
        _meta->size--;                                                     \
    } while (0)
