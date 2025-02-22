#pragma once

#include <stddef.h>

#include "sunset/vector.h"

#define RESOURCE_ID(name) _resource_id_##name
#define DECLARE_RESOURCE_ID(name) size_t RESOURCE_ID(name)
#define DEFINE_RESOURCE_ID(name, id) RESOURCE_ID(name) = id;

typedef void *(*ResourceInitFn)(void);

typedef struct ResourceManager ResourceManager;

size_t _rman_register_resource(
        ResourceManager *rman, void const *ptr, size_t size);

#define REGISTER_RESOURCE(rman, rname, value)                              \
    ({                                                                     \
        auto __local = value;                                              \
        RESOURCE_ID(rname) =                                               \
                _rman_register_resource(rman, &__local, sizeof(__local));  \
    })

typedef struct ResourceManager {
    vector(void *) resources;
} ResourceManager;

void rman_init(ResourceManager *rman_out);

void *rman_get(ResourceManager *rman, size_t id);
