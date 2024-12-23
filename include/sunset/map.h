#pragma once

#include "sunset/math.h"
#include "sunset/vector.h"
#include <stddef.h>

#define map(T) T *

#define map_init(m) vector_init(m)

static inline size_t map_find_index(void const *v,
        size_t size,
        size_t elem_size,
        void const *value,
        enum order (*compar)(void const *, void const *)) {
    size_t low = 0;
    size_t high = size;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        int cmp = compar((char *)v + mid * elem_size, value);
        if (cmp < 0) {
            low = mid + 1;
        } else if (cmp > 0) {
            high = mid;
        } else {
            return mid;
        }
    }

    return low;
}

#define map_get_index(v, value, compar)                                        \
    map_find_index(v, vector_size(v), sizeof(*v), &value, compar)

#define map_insert(v, value, compar)                                           \
    do {                                                                       \
        size_t __i =                                                           \
                map_find_index(v, vector_size(v), sizeof(*v), &value, compar); \
        vector_resize(v, vector_size(v) + 1);                                  \
        for (size_t __j = vector_size(v); __j > __i; __j--) {                  \
            v[__j] = v[__j - 1];                                               \
        }                                                                      \
        v[__i] = value;                                                        \
    } while (0)

#define map_get(v, value, compar)                                              \
    ({                                                                         \
        size_t __i =                                                           \
                map_find_index(v, vector_size(v), sizeof(*v), &value, compar); \
        __i < vector_size(v) && compar(&v[__i], &value) == ORDER_EQUAL         \
                ? &v[__i]                                                      \
                : NULL;                                                        \
    })

#define map_remove(v, value, compar)                                           \
    do {                                                                       \
        size_t __i =                                                           \
                map_find_index(v, vector_size(v), sizeof(*v), &value, compar); \
        vector_remove_index(v, __i);                                           \
    } while (0)

#define map_get_or_init(v, value, compar, value_or)                            \
    ({                                                                         \
        auto _v = map_get(v, value, compar);                                   \
        if (!_v) {                                                             \
            _v = value_or();                                                   \
            map_insert(v, _v, compar);                                         \
        }                                                                      \
        _v;                                                                    \
    })
