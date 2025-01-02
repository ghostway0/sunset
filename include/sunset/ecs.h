#ifndef SUNSET_World_H_
#define SUNSET_World_H_

#include <stddef.h>
#include <stdint.h>

#include "sunset/bitmap.h"
#include "sunset/vector.h"

#define MAX_NUM_COMPONENTS 64

#define COMPONENT_ID(type) _component_id_##type
#define DECLARE_COMPONENT_ID(type) extern size_t COMPONENT_ID(type)
#define DEFINE_COMPONENT_ID(type, id) size_t COMPONENT_ID(type) = id

#define REGISTER_COMPONENT(ecs, type)                                          \
    DEFINE_COMPONENT_ID(type, _ecs_register_component(ecs, sizeof(type)))

struct Column {
    Bitmap mask;
    size_t element_size;
    vector(uint8_t) data;
} typedef Column;

struct ArchType {
    Bitmap mask;
    size_t num_elements;
    vector(Column) columns;
} typedef ArchType;

struct EntityIndex {
    ArchType *archtype;
    size_t index;
} typedef EntityIndex;

struct World {
    size_t next_component_id;
    vector(size_t) component_sizes;
    vector(ArchType) archtypes;
    vector(EntityIndex) entity_ptrs;
} typedef World;

size_t _ecs_register_component(World *ecs, size_t component_size);

void *ecs_get_component(World *ecs, uint32_t entity_id, uint32_t component_id);

struct WorldIterator {
    World *ecs;
    Bitmap mask;
    size_t current_archtype;
    size_t current_element;
} typedef WorldIterator;

void ecs_iterator_advance(WorldIterator *iterator);

void ecs_add_entity(World *ecs, Bitmap mask);

bool ecs_iterator_is_valid(WorldIterator *iterator);

void *ecs_iterator_get_component(WorldIterator *iterator, size_t component_index);

WorldIterator ecs_iterator_create(World *ecs, Bitmap mask);

struct EntityBuilder {
    World *ecs;
    Bitmap mask;
    vector(void *) components;
    vector(size_t) component_ids;
} typedef EntityBuilder;

void entity_builder_init(EntityBuilder *builder, World *ecs);

void entity_builder_add_component(
        EntityBuilder *builder, size_t id, void *component);

void entity_builder_finish(EntityBuilder *builder);

void ecs_init(World *ecs);

#endif // SUNSET_World_H_
