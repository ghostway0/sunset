#ifndef SUNSET_ECS_H_
#define SUNSET_ECS_H_

#include <stddef.h>
#include <stdint.h>

#include "sunset/bitmask.h"
#include "sunset/vector.h"

#define ECS_MAX_COMPONENTS 64

#define COMPONENT_ID(type) _component_id_##type
#define DECLARE_COMPONENT_ID(type) extern size_t COMPONENT_ID(type)
#define DEFINE_COMPONENT_ID(type, id) size_t COMPONENT_ID(type) = id

#define REGISTER_COMPONENT(world, type)                                    \
    DEFINE_COMPONENT_ID(type, ecs_register_component(world, sizeof(type)))

struct Column {
    Bitmask mask;
    size_t element_size;
    vector(uint8_t) data;
} typedef Column;

struct Archetype {
    Bitmask mask;
    size_t num_elements;
    vector(Column) columns;
} typedef Archetype;

struct EntityPtr {
    Archetype *archetype;
    size_t index;
} typedef EntityPtr;

struct World {
    size_t next_component_id;
    vector(size_t) component_sizes;
    vector(Archetype) archetypes;
    vector(EntityPtr) entity_ptrs;
    vector(uint32_t) free_ids;
} typedef World;

size_t ecs_register_component(World *world, size_t component_size);

void *ecs_get_component(
        World *world, uint32_t entity_id, uint32_t component_id);
void ecs_remove_entity(World *world, uint32_t entity_id);

struct WorldIterator {
    World const *world;
    Bitmask mask;
    size_t current_archetype;
    size_t current_element;
} typedef WorldIterator;

void ecs_iterator_advance(WorldIterator *iterator);

uint32_t ecs_add_entity(World *world, Bitmask mask);

bool ecs_iterator_is_valid(WorldIterator const *iterator);

void *ecs_iterator_get_component(
        WorldIterator *iterator, size_t component_id);

WorldIterator ecs_iterator_create(World const *world, Bitmask mask);

void ecs_iterator_destroy(WorldIterator *iterator);

struct EntityBuilder {
    World *world;
    Bitmask mask;
    vector(void *) components;
    vector(size_t) component_ids;
} typedef EntityBuilder;

void entity_builder_init(EntityBuilder *builder, World *world);

void entity_builder_add_component(
        EntityBuilder *builder, size_t id, void *component);

void entity_builder_finish(EntityBuilder *builder);

void ecs_init(World *world);

#endif // SUNSET_ECS_H_
