#pragma once

#define map(T) T *

#define map_insert(v, value, hash)                                         \
    do {                                                                         \
        size_t i = 0;                                                          \
        while (i < vector_size(v) && hash(&v[i]) < hash(&value)) {             \
            i++;                                                             \
        }                                                                    \
        vector_resize(v, vector_size(v) + 1);                               \
        for (size_t j = vector_size(v); j > i; j--) {                        \
            v[j] = v[j - 1];                                                 \
        }                                                                    \
        v[i] = value;                                                        \
    } while (0)

#define map_get(v, value, hash)                                            \
    ({                                                                         \
        size_t i = 0;                                                          \
        while (i < vector_size(v) && hash(&v[i]) < value) {             \
            i++;                                                             \
        }                                                                    \
        i < vector_size(v) ? &v[i] : NULL;                                   \
    })
