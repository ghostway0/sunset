#pragma once

#define map(T) T *

#define map_insert(v, value, compar)                                         \
    do {                                                                         \
        size_t i = 0;                                                            \
        while (i < vector_size(v) && compar(&v[i], &value) < 0) {                \
            i++;                                                                 \
        }                                                                        \
        vector_resize(v, vector_size(v) + 1);                                   \
        for (size_t j = vector_size(v); j > i; j--) {                            \
            v[j] = v[j - 1];                                                     \
        }                                                                        \
        v[i] = value;                                                            \
    } while (0)
