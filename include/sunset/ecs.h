#ifndef ECS_H
#define ECS_H

#include <stddef.h>
#include <stdint.h>

#include "sunset/bitmap.h"
#include "sunset/vector.h"

#define MAX_NUM_COMPONENTS 64

#define ECS_COMPONENT(i) (1 << i)

#define COMPONENT_ID(type) _component_id_##type
#define DECLARE_COMPONENT_ID(type) extern size_t COMPONENT_ID(type)
#define DEFINE_COMPONENT_ID(type, id) size_t COMPONENT_ID(type) = id

#define REGISTER_COMPONENT(ecs, type)                                          \
    DEFINE_COMPONENT_ID(type, _ecs_register_component(ecs, sizeof(type)))

struct column {
    struct bitmap mask;
    size_t element_size;
    vector(uint8_t) data;
};

struct archtype {
    struct bitmap mask;
    size_t num_elements;
    vector(struct column) columns;
};

struct entity_ptr {
    struct archtype *archtype;
    size_t index;
};

struct ecs {
    size_t next_component_id;
    vector(size_t) component_sizes;
    vector(struct archtype) archtypes;
    vector(struct entity_ptr) entity_ptrs;
};

size_t _ecs_register_component(struct ecs *ecs, size_t component_size);

struct ecs_iterator {
    struct ecs *ecs;
    struct bitmap mask;
    size_t current_archtype;
    size_t current_element;
};

void ecs_iterator_advance(struct ecs_iterator *iterator);

void ecs_add_entity(struct ecs *ecs, struct bitmap mask);

bool ecs_iterator_is_valid(struct ecs_iterator *iterator);

void *ecs_iterator_get_component_raw(
        struct ecs_iterator *iterator, size_t component_index);

struct ecs_iterator ecs_iterator_create(struct ecs *ecs, struct bitmap mask);

struct entity_builder {
    struct ecs *ecs;
    struct bitmap mask;
    vector(void *) components;
    vector(size_t) component_ids;
};

void entity_builder_init(struct entity_builder *builder, struct ecs *ecs);

void entity_builder_add_component(
        struct entity_builder *builder, size_t id, void *component);

void entity_builder_finish(struct entity_builder *builder);

void ecs_init(struct ecs *ecs);

#define ecs_iterator_get_component(iterator, type)                             \
    ({                                                                         \
        void *component = NULL;                                                \
        struct archtype *arch =                                                \
                &((iterator)->ecs->archtypes[(iterator)->current_archtype]);   \
        size_t component_index = arch->component_indices[COMPONENT_##type];    \
        if (component_index != SIZE_MAX) {                                     \
            struct column *col = &arch->columns[component_index];              \
            component = col->data[(iterator)->current_element                  \
                    * col->element_size];                                      \
        }                                                                      \
        (type *)component;                                                     \
    })

#define ecs_insert(ecs, mask, component, component_index)                      \
    ecs_insert_one(ecs, mask, component, component_index)

#endif // ECS_H
