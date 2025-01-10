#pragma once

#include <stdio.h>

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
