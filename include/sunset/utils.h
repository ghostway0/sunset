#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define EPSILON 0.001

#define unused(x) ((void)(x))

#ifdef __APPLE__

#define sunset_qsort(base, nmemb, size, arg, compar)                           \
    qsort_r(base, nmemb, size, arg, compar)

#elif defined(__linux__)

#define sunset_qsort(base, nmemb, size, arg, compar)                           \
    qsort_r(base, nmemb, size, compar, arg)

#endif

#define container_of(p, T, a)                                                  \
    ((T *)((uintptr_t)(p) - (uintptr_t)(&((T *)(0))->a)))

struct timespec get_time();

uint64_t get_time_ms();

uint64_t get_time_us();

uint64_t time_since_ms(struct timespec start);

uint64_t time_since_us(struct timespec start);

static inline int compare_uint64_t(void const *a, void const *b) {
    return *(uint64_t *)a - *(uint64_t *)b;
}

#define stringify(x) #x

#define todo() assert(false && "This is not implemented yet")

#define sunset_memory(f, ...)                                                  \
    ({                                                                         \
        void *__new_ptr = f(__VA_ARGS__);                                      \
        if (__new_ptr == NULL) {                                               \
            fprintf(stderr,                                                    \
                    "%s:%d out of memory... exiting",                          \
                    __FILE__,                                                  \
                    __LINE__);                                                 \
            exit(1);                                                           \
        }                                                                      \
        __new_ptr;                                                             \
    })

#define sunset_malloc(size) sunset_memory(malloc, size);

#define sunset_calloc(num, size) sunset_memory(calloc, num, size);

#define sunset_realloc(ptr, size) sunset_memory(realloc, ptr, size);
