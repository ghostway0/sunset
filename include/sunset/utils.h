#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

struct timespec get_time(void);

uint64_t get_time_ms(void);

uint64_t get_time_us(void);

uint64_t time_since_ms(struct timespec start);

uint64_t time_since_us(struct timespec start);

float time_since_s(struct timespec start);

static inline int compare_uint64_t(void const *a, void const *b) {
    return *(uint64_t *)a - *(uint64_t *)b;
}

#define stringify(x) #x

#define forward(x) x

#define todo() assert(false && "This is not implemented yet")

#define sunset_memory(f, ...)                                              \
    ({                                                                     \
        void *__new_ptr = f(__VA_ARGS__);                                  \
        if (!__new_ptr) {                                                  \
            fprintf(stderr,                                                \
                    "%s:%d out of memory... exiting",                      \
                    __FILE__,                                              \
                    __LINE__);                                             \
            exit(1);                                                       \
        }                                                                  \
        __new_ptr;                                                         \
    })

#define sunset_malloc(size) sunset_memory(malloc, size);

#define sunset_calloc(num, size) sunset_memory(calloc, num, size);

#define sunset_realloc(ptr, size) sunset_memory(realloc, ptr, size);

#define sunset_strdup(ptr) sunset_memory(strdup, ptr);

#define swap_if(predicate, a, b)                                           \
    a = (predicate) ? (b) : (a);                                           \
    b = (predicate) ? (a) : (b);

#define SIZE_FAIL ((size_t)-1)

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
