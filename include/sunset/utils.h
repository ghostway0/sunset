#pragma once

#include <stdint.h>
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
