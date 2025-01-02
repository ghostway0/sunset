#include <log.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/sunset/vector.h"
#include "sunset/bitmask.h"
#include "sunset/vector.h"

#include "sunset/ecs.h"

static size_t archetype_num_columns(Archetype const *archetype) {
    return bitmask_popcount(&archetype->mask);
}

size_t ecs_register_component(World *world, size_t component_size) {
    size_t id = world->next_component_id++;
    assert(id < ECS_MAX_COMPONENTS);

    vector_append(world->component_sizes, component_size);
    return id;
}

static Archetype *get_archetype(World *world, Bitmask const *mask) {
    for (size_t i = 0; i < vector_size(world->archetypes); i++) {
        Archetype *archetype = &world->archetypes[i];
        if (bitmask_is_eql(&archetype->mask, mask)) {
            return archetype;
        }
    }

    return NULL;
}

static void iterator_advance_internal(WorldIterator *iterator) {
    while (iterator->current_archetype
            < vector_size(iterator->world->archetypes)) {
        Archetype *archetype =
                &iterator->world->archetypes[iterator->current_archetype];

        if (bitmask_is_superset(&archetype->mask, &iterator->mask)) {
            if (iterator->current_element < archetype->num_elements) {
                return;
            } else {
                iterator->current_archetype++;
                iterator->current_element = 0;
            }
        } else {
            iterator->current_archetype++;
        }
    }
}

void ecs_init(World *world) {
    world->next_component_id = 0;
    vector_init(world->archetypes);
    vector_init(world->component_sizes);
    vector_init(world->entity_ptrs);
    vector_init(world->free_ids);
}

void ecs_destroy(World *world) {
    for (size_t i = 0; i < vector_size(world->archetypes); i++) {
        Archetype *archetype = &world->archetypes[i];
        bitmask_destroy(&archetype->mask);
        for (size_t j = 0; j < vector_size(archetype->columns); j++) {
            vector_destroy(archetype->columns[j].data);
        }
        vector_destroy(archetype->columns);
    }
    vector_destroy(world->archetypes);
    vector_destroy(world->component_sizes);
    vector_destroy(world->entity_ptrs);
    vector_destroy(world->free_ids);
}

WorldIterator ecs_iterator_create(World const *world, Bitmask mask) {
    WorldIterator iterator = {
            .world = world,
            .mask = mask,
            .current_archetype = 0,
            .current_element = 0,
    };
    iterator_advance_internal(&iterator);
    return iterator;
}

void ecs_iterator_advance(WorldIterator *iterator) {
    if (iterator->current_element + 1
            < iterator->world->archetypes[iterator->current_archetype]
                      .num_elements) {
        iterator->current_element++;
    } else {
        iterator->current_archetype++;
        iterator->current_element = 0;
        iterator_advance_internal(iterator);
    }
}

bool ecs_iterator_is_valid(WorldIterator const *iterator) {
    return iterator->current_archetype
            < vector_size(iterator->world->archetypes);
}

void *ecs_iterator_get_component(
        WorldIterator *iterator, size_t component_id) {
    Archetype *archetype =
            &iterator->world->archetypes[iterator->current_archetype];

    size_t column_index = 0;
    size_t num_columns = archetype_num_columns(archetype);
    for (size_t i = 0; i < num_columns; i++) {
        if (bitmask_is_set(&archetype->columns[i].mask, component_id)) {
            column_index = i;
            break;
        }
    }

    Column *column = &archetype->columns[column_index];

    log_debug("11 %p %zu %p",
            column,
            iterator->current_element,
            &column->data[iterator->current_element
                    * column->element_size]);

    return &column->data[iterator->current_element * column->element_size];
}

void ecs_iterator_destroy(WorldIterator *iterator) {
    bitmask_destroy(&iterator->mask);
}

void archetype_init(World *world, Bitmask mask, Archetype *archetype_out) {
    archetype_out->mask = mask;
    archetype_out->num_elements = 0;

    vector_init(archetype_out->columns);

    for (size_t i = 0; i < ECS_MAX_COMPONENTS; i++) {
        if (bitmask_is_set(&mask, i)) {
            vector_resize(archetype_out->columns,
                    vector_size(archetype_out->columns) + 1);

            Column *column = vector_back(archetype_out->columns);

            bitmask_init_empty(world->next_component_id, &column->mask);
            bitmask_set(&column->mask, i);

            column->element_size = world->component_sizes[i];
            vector_init(column->data);
        }
    }
}

uint32_t ecs_add_entity(World *world, Bitmask mask) {
    uint32_t entity_id = vector_size(world->free_ids) > 0
            ? vector_pop_back(world->free_ids)
            : vector_size(world->entity_ptrs);

    Archetype *archetype = get_archetype(world, &mask);
    size_t entity_index = 0;

    if (archetype == NULL) {
        vector_resize(
                world->archetypes, vector_size(world->archetypes) + 1);
        archetype = vector_back(world->archetypes);
        archetype_init(world, mask, archetype);
    }

    entity_index = archetype->num_elements;
    archetype->num_elements++;

    if (entity_id >= vector_size(world->entity_ptrs)) {
        vector_resize(world->entity_ptrs, entity_id + 1);
    }

    EntityPtr new_eptr = {
            .archetype = archetype,
            .index = entity_index,
    };

    world->entity_ptrs[entity_id] = new_eptr;

    for (size_t i = 0; i < vector_size(archetype->columns); i++) {
        Column *column = &archetype->columns[i];
        vector_resize(column->data,
                archetype->num_elements * column->element_size);
    }

    return entity_id;
}

void ecs_remove_entity(World *world, uint32_t entity_id) {
    vector_append(world->free_ids, entity_id);
}

void entity_builder_init(EntityBuilder *builder, World *world) {
    builder->world = world;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &builder->mask);
    vector_init(builder->components);
    vector_init(builder->component_ids);
}

void entity_builder_destroy(EntityBuilder *builder) {
    bitmask_destroy(&builder->mask);
    for (size_t i = 0; i < vector_size(builder->components); i++) {
        free(builder->components[i]);
    }
    vector_destroy(builder->components);
    vector_destroy(builder->component_ids);
}

void entity_builder_add_component(
        EntityBuilder *builder, size_t id, void *component) {
    bitmask_set(&builder->mask, id);

    void *component_copy =
            sunset_malloc(builder->world->component_sizes[id]);
    memcpy(component_copy, component, builder->world->component_sizes[id]);

    vector_append(builder->components, component_copy);
    vector_append(builder->component_ids, id);
}

void entity_builder_finish(EntityBuilder *builder) {
    uint32_t entity_id = ecs_add_entity(builder->world, builder->mask);

    Archetype *archetype = get_archetype(builder->world, &builder->mask);
    EntityPtr *eptr = &builder->world->entity_ptrs[entity_id];

    // sanity checks
    assert(archetype && "ecs_add_entity should've added the archetype");
    assert(eptr->archetype == archetype);

    for (size_t i = 0; i < vector_size(builder->components); i++) {
        size_t component_id = builder->component_ids[i];
        void *component_data = builder->components[i];

        Column *column = NULL;
        for (size_t j = 0; j < vector_size(archetype->columns); ++j) {
            if (bitmask_is_set(&archetype->columns[j].mask, component_id)) {
                column = &archetype->columns[j];
                break;
            }
        }

        log_debug("%p %zu", column->data + eptr->index * column->element_size, eptr->index);

        assert(column && "columns should've been added earlier");

        memcpy(&column->data[eptr->index * column->element_size],
                component_data,
                column->element_size);
    }

    vector_destroy(builder->components);
    vector_destroy(builder->component_ids);
}

void *ecs_get_component(
        World *world, uint32_t entity_id, uint32_t component_id) {
    if (entity_id >= vector_size(world->entity_ptrs)) {
        return NULL;
    }

    EntityPtr eptr = world->entity_ptrs[entity_id];
    assert(eptr.archetype);

    for (size_t i = 0; i < vector_size(eptr.archetype->columns); i++) {
        if (bitmask_is_set(
                    &eptr.archetype->columns[i].mask, component_id)) {
            Column *column = &eptr.archetype->columns[i];
            return column->data + eptr.index * column->element_size;
        }
    }

    return NULL;
}
