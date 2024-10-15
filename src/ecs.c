#include "sunset/utils.h"
#include "sunset/vector.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ECS_COMPONENT(i) (1 << i)

struct column {
    uint64_t mask;
    size_t element_size;
    vector(uint8_t) data;
};

struct archtype {
    uint64_t mask;
    size_t num_elements;
    struct column *columns;
};

size_t archtype_num_columns(struct archtype const *archtype) {
    return __builtin_popcountll(archtype->mask);
}

struct ecs {
    vector(void *) systems;
    vector(struct archtype) archtypes;
};

struct archtype *ecs_get_archtype(struct ecs *ecs, uint64_t mask) {
    size_t archtypes = vector_size(ecs->archtypes);

    for (size_t i = 0; i < archtypes; ++i) {
        struct archtype *archtype = &ecs->archtypes[i];

        if (archtype->mask == mask) {
            return archtype;
        }
    }

    return NULL;
}

// void ecs_register_archtype(
//         struct ecs *ecs, uint64_t mask, size_t element_size) {
//     struct archtype *archtype = ecs_get_archtype(ecs, mask);
//
//     if (archtype == NULL) {
//         vector_append(ecs->archtypes,
//                 ((struct archtype){
//                         .mask = mask,
//                         .element_size = element_size,
//                 }));
//
//         vector_create(archtype->elements, uint8_t);
//     }
// }

// ecs_register_system(ecs, ECS_COMPONENT(0) | ECS_COMPONENT(1), sizeof(struct
// position));

struct ecs_iterator {
    struct ecs *ecs;
    uint64_t mask;
    size_t current_archtype;
    size_t current_element;
};

struct ecs_iterator ecs_iterator_create(struct ecs *ecs, uint64_t mask) {
    return (struct ecs_iterator){
            .ecs = ecs,
            .mask = mask,
            .current_archtype = 0,
            .current_element = 0,
    };
}

#define ecs_iterator_get_component(iterator, type, id)                         \
    ((type *)ecs_iterator_get_component_raw(iterator, id))

void ecs_iterator_advance(struct ecs_iterator *iterator) {
    struct archtype *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];

    iterator->current_element++;

    if (iterator->current_element >= archtype->num_elements) {
        iterator->current_element = 0;
        iterator->current_archtype++;
    }
}

int ecs_insert_one(struct ecs *ecs,
        uint64_t mask,
        void *component,
        size_t component_index) {
    struct archtype *archtype = ecs_get_archtype(ecs, mask);

    if (archtype == NULL) {
        return -1;
    }

    struct column *column = &archtype->columns[component_index];
    vector_resize(column->data, vector_size(column->data) + 1);

    memcpy(&column->data[vector_size(column->data) - 1],
            component,
            column->element_size);

    return 0;
}

#define ecs_insert(ecs, mask, component, component_index)                      \
    ecs_insert_one(ecs, mask, component, component_index)

bool ecs_iterator_is_valid(struct ecs_iterator *iterator) {
    return iterator->current_archtype < vector_size(iterator->ecs->archtypes);
}

void *ecs_iterator_get_component_raw(
        struct ecs_iterator *iterator, size_t component_index) {
    struct archtype *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];

    struct column *column = &archtype->columns[component_index];

    return &column->data[iterator->current_element * column->element_size];
}

#define ecs_iterator_get_component(iterator, type, id)                         \
    ((type *)ecs_iterator_get_component_raw(iterator, id))

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

struct static_mesh_renderer {
    vector(struct position *) positions;
    vector(struct mesh *) meshes;

    // instancing cache
};

void static_mesh_renderer_init(struct static_mesh_renderer *renderer) {
    vector_create(renderer->positions, struct position *);
    vector_create(renderer->meshes, struct mesh *);
}

void static_mesh_renderer_update(struct static_mesh_renderer *renderer) {
    size_t affected = vector_size(renderer->positions);

    for (size_t i = 0; i < affected; ++i) {
        struct position *pos = renderer->positions[i];
        struct mesh *mesh = renderer->meshes[i];

        unused(pos);
        unused(mesh);
    }
}

struct physics_system {
    vector(struct position *) positions;
    vector(struct velocity *) velocities;
};

void physics_system_init(struct physics_system *system) {
    vector_create(system->positions, struct position *);
    vector_create(system->velocities, struct velocity *);
}

void physics_system_update(struct physics_system *system) {
    size_t affected = vector_size(system->positions);

    for (size_t i = 0; i < affected; ++i) {
        struct position *pos = system->positions[i];
        struct velocity *vel = system->velocities[i];

        pos->x += vel->x;
        pos->y += vel->y;
    }
}

void ecs_init(struct ecs *ecs) {
    unused(ecs);
}
