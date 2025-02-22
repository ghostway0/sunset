#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/vector.h"

#include "sunset/rman.h"

size_t _rman_register_resource(
        ResourceManager *rman, void const *ptr, size_t size) {
    void *value_ptr = malloc(size);
    memcpy(value_ptr, ptr, size);
    vector_append(rman->resources, value_ptr);
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
