#ifndef SUNSET_ECS_H_
#define SUNSET_ECS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sunset/bitmask.h"
#include "sunset/vector.h"

#define ECS_MAX_COMPONENTS 64

#define COMPONENT_ID(type) _component_id_##type
#define DECLARE_COMPONENT_ID(type) size_t COMPONENT_ID(type)
#define DEFINE_COMPONENT_ID(type, id) COMPONENT_ID(type) = id;

typedef uint32_t Index;
typedef struct World World;

size_t _ecs_register_component(World *world, size_t component_size);

#define REGISTER_COMPONENT(world, type)                                    \
    DEFINE_COMPONENT_ID(type, _ecs_register_component(world, sizeof(type)))

struct Column {
    Bitmask mask;
    size_t element_size;
    vector(uint8_t) data;
} typedef Column;

struct Archetype {
    Bitmask mask;
    size_t num_elements;
    vector(Column) columns;
    vector(Index) free_elems;
} typedef Archetype;

struct EntityPtr {
    size_t archetype;
    size_t element;
} typedef EntityPtr;

struct World {
    vector(ptrdiff_t) component_sizes;
    vector(Archetype) archetypes;
    vector(EntityPtr) entity_ptrs;
    vector(Index) free_ids;
} typedef World;

struct EntityBuilder {
    World *world;
    Bitmask mask;
    vector(void *) components;
    vector(size_t) component_ids;
} typedef EntityBuilder;

struct WorldIterator {
    World const *world;
    Bitmask mask;
    size_t current_archetype;
    size_t current_element;
} typedef WorldIterator;

void ecs_init(World *world);

Index ecs_add_entity(World *world, Bitmask mask);
void ecs_remove_entity(World *world, uint32_t entity_id);

void *ecs_get_component(
        World *world, uint32_t entity_id, uint32_t component_id);

void entity_builder_init(EntityBuilder *builder, World *world);
void entity_builder_add_component(
        EntityBuilder *builder, size_t id, void *component);
Index entity_builder_finish(EntityBuilder *builder);

WorldIterator worldit_create(World const *world, Bitmask mask);
bool worldit_is_valid(WorldIterator const *iterator);
void worldit_advance(WorldIterator *iterator);
void *worldit_get_component(WorldIterator *iterator, size_t component_id);
void worldit_destroy(WorldIterator *iterator);

#endif // SUNSET_ECS_H_
