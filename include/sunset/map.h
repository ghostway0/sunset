#pragma once

#define map(T) T *

#define map_insert(v, value, hash)                                             \
    do {                                                                       \
        size_t i = 0;                                                          \
        while (i < vector_size(v) && hash(&v[i]) < hash(&value)) {             \
            i++;                                                               \
        }                                                                      \
        vector_resize(v, vector_size(v) + 1);                                  \
        for (size_t j = vector_size(v); j > i; j--) {                          \
            v[j] = v[j - 1];                                                   \
        }                                                                      \
        v[i] = value;                                                          \
    } while (0)

#define map_get(v, value, hash)                                                \
    ({                                                                         \
        size_t i = 0;                                                          \
        while (i < vector_size(v) && hash(&v[i]) < value) {                    \
            i++;                                                               \
        }                                                                      \
        i < vector_size(v) && hash(&v[i]) == value ? &v[i] : NULL;             \
    })

#define map_get_or_init(v, value, hash, value_or)                              \
    ({                                                                         \
        auto _v = map_get(v, value, hash);                                     \
        if (!_v) {                                                             \
            _v = value_or();                                                   \
            map_insert(v, _v, hash);                                           \
        }                                                                      \
        _v;                                                                    \
    })
