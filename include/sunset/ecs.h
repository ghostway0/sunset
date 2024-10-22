#ifndef ECS_H
#define ECS_H

#include "vector.h"
#include <stddef.h>
#include <stdint.h>

#define ECS_COMPONENT(i) (1 << i)

struct column {
    uint64_t mask;
    size_t element_size;
    vector(uint8_t) data;
};

struct archtype {
    uint64_t mask;
    size_t num_elements;
    vector(struct column) columns;
};

struct ecs {
    vector(void *) systems;
    vector(struct archtype) archtypes;
};

struct ecs_iterator {
    struct ecs *ecs;
    uint64_t mask;
    size_t current_archtype;
    size_t current_element;
};

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

struct static_mesh_renderer {
    vector(struct position *) positions;
    vector(struct mesh *) meshes;
};

struct physics_system {
    vector(struct position *) positions;
    vector(struct velocity *) velocities;
};

// Function Prototypes
size_t archtype_num_columns(struct archtype const *archtype);
struct archtype *ecs_get_archtype(struct ecs *ecs, uint64_t mask);
void ecs_iterator_advance(struct ecs_iterator *iterator);
int ecs_insert_one(struct ecs *ecs,
        uint64_t mask,
        void *component,
        size_t component_index);
bool ecs_iterator_is_valid(struct ecs_iterator *iterator);
void *ecs_iterator_get_component_raw(
        struct ecs_iterator *iterator, size_t component_index);
struct ecs_iterator ecs_iterator_create(struct ecs *ecs, uint64_t mask);
int ecs_add_component(struct ecs *ecs,
        uint64_t mask,
        void *component,
        size_t component_index);

struct entity_builder {
    struct ecs *ecs;
    uint64_t mask;
    vector(void *) components;
    vector(size_t) component_ids;
};

void entity_builder_init(struct entity_builder *builder, struct ecs *ecs);

void entity_builder_add_component(
        struct entity_builder *builder, void *component, size_t id);

void ecs_add_entity(struct ecs *ecs, uint64_t mask);

void static_mesh_renderer_init(struct static_mesh_renderer *renderer);
void static_mesh_renderer_update(struct static_mesh_renderer *renderer);

void physics_system_init(struct physics_system *system);
void physics_system_update(struct physics_system *system);

int entity_builder_finish(struct entity_builder *builder);

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
