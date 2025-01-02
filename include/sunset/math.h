#ifndef MATH_H
#define MATH_H

#include "sunset/utils.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

#define max(a, b) ((a) > (b) ? (a) : (b))

#define clamp(x, lower, upper) (min(max((x), (lower)), (upper)))

#define within(x, lower, upper) ((x) >= (lower) && (x) <= (upper))

#define top_percentile(arr, n, p, compare)                                 \
    ({                                                                     \
        size_t *sorted = sunset_malloc(n * sizeof(size_t));                \
        memcpy(sorted, arr, n * sizeof(size_t));                           \
        qsort(sorted, n, sizeof(size_t), compare);                         \
        size_t idx = n - n * p / 100;                                      \
        size_t result = sorted[idx];                                       \
        free(sorted);                                                      \
        result;                                                            \
    })

enum order {
    ORDER_LESS_THAN = -1,
    ORDER_EQUAL = 0,
    ORDER_GREATER_THAN = 1,
};

#define vectors_close(type, a, b, tolerance)                               \
    ({                                                                     \
        size_t _size = sizeof(type) / sizeof((a)[0]);                      \
        double _sum = 0.0;                                                 \
        for (size_t i = 0; i < _size; i++) {                               \
            double _diff = (a)[i] - (b)[i];                                \
            _sum += _diff * _diff;                                         \
        }                                                                  \
        sqrt(_sum) <= (tolerance);                                         \
    })

#define is_zero_vector(type, vector, tolerance)                            \
    ({                                                                     \
        type _zero;                                                        \
        memset(&_zero, 0, sizeof(_zero));                                  \
        vectors_close(type, vector, _zero, tolerance);                     \
    })

static inline enum order compare_ptrs(void const *a, void const *b) {
    if (a > b) {
        return ORDER_GREATER_THAN;
    }

    if (a < b) {
        return ORDER_LESS_THAN;
    }

    return ORDER_EQUAL;
}

#endif // MATH_H
