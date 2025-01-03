#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/bitmap.h"
#include "sunset/vector.h"

#include "sunset/ecs.h"

static size_t archtype_num_columns(struct archtype const *archtype) {
    return bitmap_popcount(&archtype->mask);
}

size_t _ecs_register_component(struct ecs *ecs, size_t component_size) {
    size_t id = ecs->next_component_id++;
    assert(id < MAX_NUM_COMPONENTS);

    vector_append(ecs->component_sizes, component_size);
    return id;
}

static struct archtype *get_archtype(struct ecs *ecs, struct bitmap *mask) {
    for (size_t i = 0; i < vector_size(ecs->archtypes); ++i) {
        struct archtype *archtype = &ecs->archtypes[i];
        if (bitmap_is_eql(&archtype->mask, mask)) {
            return archtype;
        }
    }

    return NULL;
}

static void ecs_iterator_advance_internal(struct ecs_iterator *iterator) {
    while (iterator->current_archtype < vector_size(iterator->ecs->archtypes)) {
        struct archtype *archtype =
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

void ecs_init(struct ecs *ecs) {
    ecs->next_component_id = 0;
    vector_init(ecs->archtypes);
    vector_init(ecs->component_sizes);
    vector_init(ecs->entity_ptrs);
}

void ecs_destroy(struct ecs *ecs) {
    for (size_t i = 0; i < vector_size(ecs->archtypes); i++) {
        struct archtype *archtype = &ecs->archtypes[i];
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

struct ecs_iterator ecs_iterator_create(struct ecs *ecs, struct bitmap mask) {
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

bool ecs_iterator_is_valid(struct ecs_iterator *iterator) {
    return iterator->current_archtype < vector_size(iterator->ecs->archtypes);
}

void *ecs_iterator_get_component(
        struct ecs_iterator *iterator, size_t component_id) {
    struct archtype *archtype =
            &iterator->ecs->archtypes[iterator->current_archtype];

    size_t column_index = 0;
    size_t num_columns = archtype_num_columns(archtype);
    for (size_t i = 0; i < num_columns; ++i) {
        if (bitmap_is_set(&archtype->columns[i].mask, component_id)) {
            column_index = i;
            break;
        }
    }

    struct column *column = &archtype->columns[column_index];

    return &column->data[iterator->current_element * column->element_size];
}

void archtype_init(
        struct ecs *ecs, struct bitmap mask, struct archtype *archtype_out) {
    archtype_out->mask = mask;
    archtype_out->num_elements = 0;

    vector_init(archtype_out->columns);

    for (size_t i = 0; i < MAX_NUM_COMPONENTS; ++i) {
        if (bitmap_is_set(&mask, i)) {
            vector_resize(archtype_out->columns,
                    vector_size(archtype_out->columns) + 1);

            struct column *column = vector_back(archtype_out->columns);

            bitmap_init_empty(ecs->next_component_id, &column->mask);
            bitmap_set(&column->mask, i);

            column->element_size = ecs->component_sizes[i];
            vector_init(column->data);
        }
    }
}

void ecs_add_entity(struct ecs *ecs, struct bitmap mask) {
    struct archtype *archtype = get_archtype(ecs, &mask);
    size_t entity_index = 0;

    if (archtype == NULL) {
        vector_resize(ecs->archtypes, vector_size(ecs->archtypes) + 1);
        archtype = vector_back(ecs->archtypes);

        archtype_init(ecs, mask, archtype);
    }

    entity_index = archtype->num_elements;
    archtype->num_elements++;

    vector_append(ecs->entity_ptrs,
            ((struct entity_ptr){
                    .archtype = archtype,
                    .index = entity_index,
            }));

    for (size_t i = 0; i < vector_size(archtype->columns); ++i) {
        struct column *column = &archtype->columns[i];
        vector_resize(
                column->data, archtype->num_elements * column->element_size);
    }
}

void entity_builder_init(struct entity_builder *builder, struct ecs *ecs) {
    builder->ecs = ecs;
    bitmap_init_empty(MAX_NUM_COMPONENTS, &builder->mask);
    vector_init(builder->components);
    vector_init(builder->component_ids);
}

void entity_builder_destroy(struct entity_builder *builder) {
    bitmap_destroy(&builder->mask);
    for (size_t i = 0; i < vector_size(builder->components); ++i) {
        free(builder->components[i]);
    }
    vector_destroy(builder->components);
    vector_destroy(builder->component_ids);
}

void entity_builder_add_component(
        struct entity_builder *builder, size_t id, void *component) {
    bitmap_set(&builder->mask, id);

    void *component_copy = sunset_malloc(builder->ecs->component_sizes[id]);
    memcpy(component_copy, component, builder->ecs->component_sizes[id]);

    vector_append(builder->components, component_copy);
    vector_append(builder->component_ids, id);
}

void entity_builder_finish(struct entity_builder *builder) {
    ecs_add_entity(builder->ecs, builder->mask);

    struct entity_ptr *eptr = vector_back(builder->ecs->entity_ptrs);

    struct archtype *archtype = get_archtype(builder->ecs, &builder->mask);
    assert(archtype && "ecs_add_entity should've added the archtype");

    for (size_t i = 0; i < vector_size(builder->components); ++i) {
        size_t component_id = builder->component_ids[i];
        void *component_data = builder->components[i];

        struct column *column = NULL;
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
        struct ecs *ecs, uint32_t entity_id, uint32_t component_id) {
    if (entity_id >= vector_size(ecs->entity_ptrs)) {
        return NULL;
    }

    struct entity_ptr eptr = ecs->entity_ptrs[entity_id];

    struct column *column = NULL;
    for (size_t i = 0; i < vector_size(eptr.archtype->columns); i++) {
        if (bitmap_is_set(&eptr.archtype->columns[i].mask, component_id)) {
            column = &eptr.archtype->columns[i];
            break;
        }
    }

    if (!column) {
        return NULL;
    }

    return column->data + eptr.index * column->element_size;
}
