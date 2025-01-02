#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/bitmap.h"
#include "sunset/vector.h"

#include "sunset/ecs.h"

static size_t archtype_num_columns(ArchType const *archtype) {
    return bitmap_popcount(&archtype->mask);
}

size_t _ecs_register_component(World *ecs, size_t component_size) {
    size_t id = ecs->next_component_id++;
    assert(id < MAX_NUM_COMPONENTS);

    vector_append(ecs->component_sizes, component_size);
    return id;
}

static ArchType *get_archtype(World *ecs, Bitmap *mask) {
    for (size_t i = 0; i < vector_size(ecs->archtypes); ++i) {
        ArchType *archtype = &ecs->archtypes[i];
        if (bitmap_is_eql(&archtype->mask, mask)) {
            return archtype;
        }
    }

    return NULL;
}

static void ecs_iterator_advance_internal(WorldIterator *iterator) {
    while (iterator->current_archtype < vector_size(iterator->ecs->archtypes)) {
        ArchType *archtype =
                &iterator->ecs->archtypes[iterator->current_archtype];

        if (bitmap_is_superset(&archtype->mask, &iterator->mask)) {
            if (iterator->current_element < archtype->num_elements) {
                return;
            } else {
                iterator->current_archtype++;
                iterator->current_element = 0;
            }
        } else {
            iterator->current_archtype++;
        }
    }
}

void ecs_init(World *ecs) {
    ecs->next_component_id = 0;
    vector_init(ecs->archtypes);
    vector_init(ecs->component_sizes);
    vector_init(ecs->entity_ptrs);
}

void ecs_destroy(World *ecs) {
    for (size_t i = 0; i < vector_size(ecs->archtypes); i++) {
        ArchType *archtype = &ecs->archtypes[i];
        bitmap_destroy(&archtype->mask);
        for (size_t j = 0; j < vector_size(archtype->columns); j++) {
            vector_destroy(archtype->columns[j].data);
        }
        vector_destroy(archtype->columns);
    }
    vector_destroy(ecs->archtypes);
    vector_destroy(ecs->component_sizes);
    vector_destroy(ecs->entity_ptrs);
}

WorldIterator ecs_iterator_create(World *ecs, Bitmap mask) {
    WorldIterator iterator = {
            .ecs = ecs,
            .mask = mask,
            .current_archtype = 0,
            .current_element = 0,
    };
    ecs_iterator_advance_internal(&iterator);
    return iterator;
}

void ecs_iterator_advance(WorldIterator *iterator) {
    if (iterator->current_element + 1
            < iterator->ecs->archtypes[iterator->current_archtype]
                    .num_elements) {
        iterator->current_element++;
    } else {
        iterator->current_archtype++;
        iterator->current_element = 0;
        ecs_iterator_advance_internal(iterator);
    }
}

bool ecs_iterator_is_valid(WorldIterator *iterator) {
    return iterator->current_archtype < vector_size(iterator->ecs->archtypes);
}

void *ecs_iterator_get_component(
        WorldIterator *iterator, size_t component_id) {
    ArchType *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];

    size_t column_index = 0;
    size_t num_columns = archtype_num_columns(archtype);
    for (size_t i = 0; i < num_columns; ++i) {
        if (bitmap_is_set(&archtype->columns[i].mask, component_id)) {
            column_index = i;
            break;
        }
    }

    Column *column = &archtype->columns[column_index];

    return &column->data[iterator->current_element * column->element_size];
}

void archtype_init(
        World *ecs, Bitmap mask, ArchType *archtype_out) {
    archtype_out->mask = mask;
    archtype_out->num_elements = 0;

    vector_init(archtype_out->columns);

    for (size_t i = 0; i < MAX_NUM_COMPONENTS; ++i) {
        if (bitmap_is_set(&mask, i)) {
            vector_resize(archtype_out->columns,
                    vector_size(archtype_out->columns) + 1);

            Column *column = vector_back(archtype_out->columns);

            bitmap_init_empty(ecs->next_component_id, &column->mask);
            bitmap_set(&column->mask, i);

            column->element_size = ecs->component_sizes[i];
            vector_init(column->data);
        }
    }
}

void ecs_add_entity(World *ecs, Bitmap mask) {
    ArchType *archtype = get_archtype(ecs, &mask);
    size_t entity_index = 0;

    if (archtype == NULL) {
        vector_resize(ecs->archtypes, vector_size(ecs->archtypes) + 1);
        archtype = vector_back(ecs->archtypes);

        archtype_init(ecs, mask, archtype);
    }

    entity_index = archtype->num_elements;
    archtype->num_elements++;

    vector_append(ecs->entity_ptrs,
            ((EntityIndex){
                    .archtype = archtype,
                    .index = entity_index,
            }));

    for (size_t i = 0; i < vector_size(archtype->columns); ++i) {
        Column *column = &archtype->columns[i];
        vector_resize(
                column->data, archtype->num_elements * column->element_size);
    }
}

void entity_builder_init(EntityBuilder *builder, World *ecs) {
    builder->ecs = ecs;
    bitmap_init_empty(MAX_NUM_COMPONENTS, &builder->mask);
    vector_init(builder->components);
    vector_init(builder->component_ids);
}

void entity_builder_destroy(EntityBuilder *builder) {
    bitmap_destroy(&builder->mask);
    for (size_t i = 0; i < vector_size(builder->components); ++i) {
        free(builder->components[i]);
    }
    vector_destroy(builder->components);
    vector_destroy(builder->component_ids);
}

void entity_builder_add_component(
        EntityBuilder *builder, size_t id, void *component) {
    bitmap_set(&builder->mask, id);

    void *component_copy = sunset_malloc(builder->ecs->component_sizes[id]);
    memcpy(component_copy, component, builder->ecs->component_sizes[id]);

    vector_append(builder->components, component_copy);
    vector_append(builder->component_ids, id);
}

void entity_builder_finish(EntityBuilder *builder) {
    ecs_add_entity(builder->ecs, builder->mask);

    EntityIndex *eptr = vector_back(builder->ecs->entity_ptrs);

    ArchType *archtype = get_archtype(builder->ecs, &builder->mask);
    assert(archtype && "ecs_add_entity should've added the archtype");

    for (size_t i = 0; i < vector_size(builder->components); ++i) {
        size_t component_id = builder->component_ids[i];
        void *component_data = builder->components[i];

        Column *column = NULL;
        for (size_t j = 0; j < vector_size(archtype->columns); ++j) {
            if (bitmap_is_set(&archtype->columns[j].mask, component_id)) {
                column = &archtype->columns[j];
                break;
            }
        }

        assert(column && "columns should've been added earlier");

        memcpy(&column->data[eptr->index * column->element_size],
                component_data,
                column->element_size);
    }
}

void *ecs_get_component(
        World *ecs, uint32_t entity_id, uint32_t component_id) {
    if (entity_id >= vector_size(ecs->entity_ptrs)) {
        return NULL;
    }

    EntityIndex eptr = ecs->entity_ptrs[entity_id];

    for (size_t i = 0; i < vector_size(eptr.archtype->columns); i++) {
        if (bitmap_is_set(&eptr.archtype->columns[i].mask, component_id)) {
            Column *column = &eptr.archtype->columns[i];
            return column->data + eptr.index * column->element_size;
        }
    }

    return NULL;
}
