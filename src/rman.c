#include <stddef.h>

#include "sunset/vector.h"

#include "sunset/rman.h"

size_t _rman_register_resource(ResourceManager *rman, void *ptr) {
    vector_append(rman->resources, ptr);
    return vector_size(rman->resources) - 1;
}

void rman_init(ResourceManager *rman_out) {
    vector_init(rman_out->resources);
}

void *rman_get(ResourceManager *rman, size_t id) {
    if (id >= vector_size(rman->resources)) {
        return NULL;
    }

    return rman->resources[id];
}
