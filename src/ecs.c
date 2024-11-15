#include "sunset/ecs.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t components_size[] = {
        sizeof(struct position),
        sizeof(struct velocity),
};

size_t archtype_num_columns(struct archtype const *archtype) {
    return __builtin_popcountll(archtype->mask);
}

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

// ecs_register_system(ecs, ECS_COMPONENT(0) | ECS_COMPONENT(1), sizeof(struct
// position));

static void ecs_iterator_advance_internal(struct ecs_iterator *iterator) {
    struct archtype *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];
    while (iterator->current_archtype < vector_size(iterator->ecs->archtypes)
            && (archtype->mask & iterator->mask) != iterator->mask) {
        iterator->current_archtype++;
        archtype = &iterator->ecs->archtypes[iterator->current_archtype];
    }

    iterator->current_element = 0;
}

struct ecs_iterator ecs_iterator_create(struct ecs *ecs, uint64_t mask) {
    struct ecs_iterator iterator = {
            .ecs = ecs,
            .mask = mask,
            .current_archtype = 0,
            .current_element = 0,
    };

    ecs_iterator_advance_internal(&iterator);

    return iterator;
}

void ecs_iterator_advance(struct ecs_iterator *iterator) {
    if (iterator->current_element
            < iterator->ecs->archtypes[iterator->current_archtype].num_elements
                    - 1) {
        iterator->current_element++;
        return;
    }

    iterator->current_archtype++;
    ecs_iterator_advance_internal(iterator);
}

void entity_builder_init(struct entity_builder *builder, struct ecs *ecs) {
    builder->ecs = ecs;
    builder->mask = 0;

    vector_init(builder->components);
    vector_init(builder->component_ids);
}

void entity_builder_add_component(
        struct entity_builder *builder, void *component, size_t id) {
    vector_append(builder->components, component);
    builder->mask |= ECS_COMPONENT(id);
    vector_append(builder->component_ids, id);
}

void ecs_add_entity(struct ecs *ecs, uint64_t mask) {
    struct archtype *archtype = ecs_get_archtype(ecs, mask);

    if (archtype != NULL) {
        archtype->num_elements++;
        return;
    }

    vector_append(ecs->archtypes,
            ((struct archtype){
                    .mask = mask,
                    .num_elements = 0,
            }));

    archtype = &ecs->archtypes[vector_size(ecs->archtypes) - 1];

    vector_init(archtype->columns);

    while (mask) {
        vector_append(archtype->columns,
                ((struct column){
                        .element_size = 0,
                        .data = NULL,
                }));

        struct column *column =
                &archtype->columns[vector_size(archtype->columns) - 1];

        column->mask = mask & -mask;
        column->element_size = components_size[__builtin_ctzll(column->mask)];

        vector_init(column->data);

        mask &= mask - 1;
    }

    archtype->num_elements++;
}

int entity_builder_finish(struct entity_builder *builder) {
    size_t num_components = vector_size(builder->components);

    ecs_add_entity(builder->ecs, builder->mask);

    for (size_t i = 0; i < num_components; ++i) {
        if (ecs_add_component(builder->ecs,
                    builder->mask,
                    builder->components[i],
                    builder->component_ids[i])) {
            return -1;
        }
    }

    vector_free(builder->components);
    vector_free(builder->component_ids);

    return 0;
}

int ecs_add_component(struct ecs *ecs,
        uint64_t mask,
        void *component,
        size_t component_index) {
    struct archtype *archtype = ecs_get_archtype(ecs, mask);

    if (archtype == NULL) {
        return -1;
    }

    struct column *column = &archtype->columns[component_index];

    vector_resize(column->data, vector_size(column->data) + 1);

    memcpy(&column->data[(vector_size(column->data) - 1)
                   * column->element_size],
            component,
            column->element_size);

    return 0;
}

bool ecs_iterator_is_valid(struct ecs_iterator *iterator) {
    return iterator->current_archtype < vector_size(iterator->ecs->archtypes)
            && iterator->current_element
            < iterator->ecs->archtypes[iterator->current_archtype].num_elements;
}

void *ecs_iterator_get_component_raw(
        struct ecs_iterator *iterator, size_t component_index) {
    struct archtype *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];

    struct column *column = &archtype->columns[component_index];

    return &column->data[iterator->current_element * column->element_size];
}

void static_mesh_renderer_init(struct static_mesh_renderer *renderer) {
    vector_init(renderer->positions);
    vector_init(renderer->meshes);
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

void physics_system_init(struct physics_system *system) {
    vector_init(system->positions);
    vector_init(system->velocities);
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
    vector_init(ecs->systems);
    vector_init(ecs->archtypes);
}
