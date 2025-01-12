#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define EPSILON 0.001

#define unused(x) ((void)(x))

#ifdef __APPLE__

#define sunset_qsort(base, nmemb, size, arg, compar)                       \
    qsort_r(base, nmemb, size, arg, compar)

#elif defined(__linux__)

#define sunset_qsort(base, nmemb, size, arg, compar)                       \
    qsort_r(base, nmemb, size, compar, arg)

#endif

#define container_of(p, T, a)                                              \
    ((T *)((uintptr_t)(p) - (uintptr_t)(&((T *)(0))->a)))

static inline int compare_uint64_t(void const *a, void const *b) {
    return *(uint64_t *)a - *(uint64_t *)b;
}

#define stringify(x) #x

#define forward(x) x

#define todo() assert(false && "This is not implemented yet")

#define one_matches(value, ...)                                            \
    ({                                                                     \
        bool _result = false;                                              \
        const typeof(value) _val = (value);                                \
        const typeof(_val) _values[] = {__VA_ARGS__};                      \
        const size_t _count = sizeof(_values) / sizeof(_values[0]);        \
        for (size_t _i = 0; _i < _count; _i++) {                           \
            if (_val == _values[_i]) {                                     \
                _result = true;                                            \
                break;                                                     \
            }                                                              \
        }                                                                  \
        _result;                                                           \
    })

#define all_match(...)                                                     \
    ({                                                                     \
        bool _result = false;                                              \
        const typeof(__VA_ARGS__) _values[] = {__VA_ARGS__};               \
        const size_t _count = sizeof(_values) / sizeof(_values[0]);        \
        if (_count >= 2) {                                                 \
            _result = true;                                                \
            const typeof(_values[0]) _first = _values[0];                  \
            for (size_t _i = 1; _i < _count; _i++) {                       \
                if (_first != _values[_i]) {                               \
                    _result = false;                                       \
                    break;                                                 \
                }                                                          \
            }                                                              \
        }                                                                  \
        _result;                                                           \
    })

typedef intmax_t ssize_t;

#define sunset_flag(i) (1 << i)

void memswap(void *a, void *b, size_t n);
