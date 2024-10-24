#pragma once

#include <stdint.h>

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
