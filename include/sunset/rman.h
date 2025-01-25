#pragma once

#include <stddef.h>

#include "sunset/vector.h"

#define RESOURCE_ID(name) _resource_id_##name
#define DECLARE_RESOURCE_ID(name) size_t RESOURCE_ID(name)
#define DEFINE_RESOURCE_ID(name, id) RESOURCE_ID(name) = id;

typedef void *(*ResourceInitFn)(void);

typedef struct ResourceManager ResourceManager;

size_t _rman_register_resource(ResourceManager *rman, void *ptr);

#define REGISTER_RESOURCE(rman, rname, ptr)                                \
    RESOURCE_ID(rname) = _rman_register_resource(rman, ptr)

typedef struct ResourceManager {
    vector(void *) resources;
} ResourceManager;

void rman_init(ResourceManager *rman_out);

void *rman_get(ResourceManager *rman, size_t id);

#define rman_get_or_init(rman, name, rinit)                                \
    ({                                                                     \
        void *__ptr = rman_get(rman, RESOURCE_ID(name));                   \
        if (!__ptr) {                                                      \
            _rman_register_resource(rman, rinit());                        \
        }                                                                  \
        __ptr;                                                             \
    })
