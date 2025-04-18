#include <assert.h>
#include <enet/enet.h>
#include <stdint.h>
#include <string.h>

#include "rman.h"
#include "sunset/bitmask.h"
#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/vector.h"

#define MAX_NETWORK_IDS (1 << 16)

typedef uint16_t NetworkId;

typedef enum PacketCommand {
    CMD_CREATE = 1,
    CMD_MODIFY = 2
} PacketCommand;

typedef struct EntityHeader {
    NetworkId net_id;
    PacketCommand command;
    uint32_t data_size;
    Bitmask components;
} __attribute__((packed)) EntityHeader;

typedef struct NetworkSync {
    Bitmask to_sync;
    bool is_synced;
    NetworkId net_id;
} NetworkSync;

DECLARE_COMPONENT_ID(NetworkSync);

typedef struct SyncPacket {
    vector(vector(uint8_t)) data;
    vector(EntityHeader) headers;
    vector(NetworkId) entity_ids;
    uint32_t seq_num;
} SyncPacket;

static EntityPtr g_netid_table[MAX_NETWORK_IDS];
static uint32_t g_seq_num = 0;
static uint32_t g_last_seq_num = 0;

DECLARE_RESOURCE_ID(host_enet);

static void netid_table_init(void) {
    memset(g_netid_table, 0, sizeof(g_netid_table));
}

static NetworkId assign_netid(EntityPtr eptr) {
    static NetworkId next_id = 0;
    NetworkId id = next_id++;
    if (next_id == 0) {
        assert(false && "Network ID overflow");
    }
    g_netid_table[id] = eptr;
    return id;
}

static void sync_packet_destroy(SyncPacket *packet) {
    for (size_t i = 0; i < vector_size(packet->data); i++) {
        vector_destroy(packet->data[i]);
    }
    vector_destroy(packet->data);
    vector_destroy(packet->headers);
    vector_destroy(packet->entity_ids);
}

static bool serialize_entity(World *world,
        EntityPtr eptr,
        EntityHeader *header,
        vector(uint8_t) * out) {
    NetworkSync *sync =
            ecs_component_from_ptr(world, eptr, COMPONENT_ID(NetworkSync));
    if (!sync) {
        return false;
    }

    Bitmask const *entity_bitmask = ecs_get_entity_archetype(world, eptr);
    header->net_id = sync->net_id;
    header->command = sync->is_synced ? CMD_MODIFY : CMD_CREATE;
    header->data_size = 0;
    header->components = bitmask_clone(
            sync->is_synced ? &sync->to_sync : entity_bitmask);

    vector(uint8_t) temp;
    vector_init(temp);

    Bitmask header_bitmask = header->components;
    Bitmask comps = bitmask_clone(&header_bitmask);
    while (!bitmask_is_zero(&comps)) {
        size_t comp_id = bitmask_ctz(&comps);
        size_t comp_size = ecs_get_component_size(world, comp_id);
        if (comp_size == SIZE_MAX) {
            bitmask_destroy(&comps);
            vector_destroy(temp);
            return false;
        }
        uint8_t *value = ecs_component_from_ptr(world, eptr, comp_id);
        vector_append_multiple(temp, value, comp_size);
        header->data_size += comp_size;
        bitmask_lsb_reset(&comps);
    }
    bitmask_destroy(&comps);

    vector_append_multiple(*out, (uint8_t *)header, sizeof(EntityHeader));
    vector_append_multiple(*out, temp, vector_size(temp));
    vector_destroy(temp);

    sync->is_synced = true;
    return true;
}

static SyncPacket create_sync_packet(World *world) {
    SyncPacket packet = {0};
    vector_init(packet.data);
    vector_init(packet.headers);
    vector_init(packet.entity_ids);
    packet.seq_num = g_seq_num++;

    Bitmask itmask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &itmask);
    bitmask_set(&itmask, COMPONENT_ID(NetworkSync));

    WorldIterator it = worldit_create(world, itmask);
    while (worldit_is_valid(&it)) {
        EntityPtr eptr = worldit_get_entityptr(&it);
        NetworkSync *sync = ecs_component_from_ptr(
                world, eptr, COMPONENT_ID(NetworkSync));

        if (bitmask_is_zero(&sync->to_sync) && sync->is_synced) {
            worldit_advance(&it);
            continue;
        }

        vector(uint8_t) entity_data;
        vector_init(entity_data);
        EntityHeader header = {0};

        if (serialize_entity(world, eptr, &header, &entity_data)) {
            vector_append(packet.data, entity_data);
            vector_append(packet.headers, header);
            vector_append(packet.entity_ids, sync->net_id);
        } else {
            vector_destroy(entity_data);
        }

        worldit_advance(&it);
    }

    worldit_destroy(&it);
    bitmask_destroy(&itmask);
    return packet;
}

static bool deserialize_create_entity(World *world,
        EntityHeader *header,
        uint8_t *data,
        size_t data_len) {
    if (header->data_size > data_len) {
        return false;
    }

    EntityPtr eptr = g_netid_table[header->net_id];
    if (!eptr_eql(eptr, ENTITY_PTR_INVALID)) {
        ecs_remove_ptr(world, eptr);
    }

    EntityBuilder builder;
    entity_builder_init(&builder, world);

    Bitmask header_bitmask = header->components;
    NetworkSync sync = {.to_sync = bitmask_clone(&header_bitmask),
            .is_synced = true,
            .net_id = header->net_id};
    entity_builder_add(&builder, COMPONENT_ID(NetworkSync), &sync);

    Bitmask comps = bitmask_clone(&header_bitmask);
    size_t offset = 0;

    while (!bitmask_is_zero(&comps)) {
        size_t comp_id = bitmask_ctz(&comps);
        size_t comp_size = ecs_get_component_size(world, comp_id);
        if (comp_size == SIZE_MAX
                || offset + comp_size > header->data_size) {
            bitmask_destroy(&comps);
            entity_builder_destroy(&builder);
            return false;
        }
        entity_builder_add(&builder, comp_id, data + offset);
        offset += comp_size;
        bitmask_lsb_reset(&comps);
    }

    bitmask_destroy(&comps);
    eptr = world->entity_ptrs[entity_builder_finish(&builder)];
    g_netid_table[header->net_id] = eptr;
    return true;
}

static bool deserialize_modify_entity(World *world,
        EntityHeader *header,
        uint8_t *data,
        size_t data_len) {
    if (header->data_size > data_len) {
        return false;
    }

    EntityPtr eptr = g_netid_table[header->net_id];
    if (eptr_eql(eptr, ENTITY_PTR_INVALID)) {
        return false;
    }

    Bitmask header_bitmask = header->components;
    Bitmask comps = bitmask_clone(&header_bitmask);
    size_t offset = 0;

    while (!bitmask_is_zero(&comps)) {
        size_t comp_id = bitmask_ctz(&comps);
        size_t comp_size = ecs_get_component_size(world, comp_id);
        if (comp_size == SIZE_MAX
                || offset + comp_size > header->data_size) {
            bitmask_destroy(&comps);
            return false;
        }
        uint8_t *dest = ecs_component_from_ptr(world, eptr, comp_id);
        if (dest) {
            memcpy(dest, data + offset, comp_size);
        }
        offset += comp_size;
        bitmask_lsb_reset(&comps);
    }

    bitmask_destroy(&comps);
    return true;
}

static void deserialize_packet(
        World *world, uint8_t *data, size_t data_len, uint32_t seq_num) {
    if (seq_num <= g_last_seq_num || data_len < sizeof(EntityHeader)) {
        return;
    }
    g_last_seq_num = seq_num;

    size_t offset = 0;
    while (offset + sizeof(EntityHeader) <= data_len) {
        EntityHeader header;
        memcpy(&header, data + offset, sizeof(EntityHeader));
        offset += sizeof(EntityHeader);

        if (header.command == CMD_CREATE) {
            deserialize_create_entity(
                    world, &header, data + offset, data_len - offset);
        } else if (header.command == CMD_MODIFY) {
            deserialize_modify_entity(
                    world, &header, data + offset, data_len - offset);
        } else {
            break; // Invalid command
        }

        offset += header.data_size;
    }
}

bool network_init(EngineContext *engine_context,
        char const *remote_addr,
        uint16_t remote_port) {
    REGISTER_COMPONENT(&engine_context->world, NetworkSync);
    netid_table_init();

    if (enet_initialize() != 0) {
        return false;
    }

    ENetHost *host =
            enet_host_create(NULL, 32, 2, 0, 0); // 32 peers, 2 channels
    if (!host) {
        enet_deinitialize();
        return false;
    }
    REGISTER_RESOURCE(&engine_context->rman, host_enet, host);

    ENetAddress address;
    enet_address_set_host(&address, remote_addr);
    address.port = remote_port;
    ENetPeer *peer = enet_host_connect(host, &address, 2, 0);
    if (!peer) {
        enet_host_destroy(host);
        enet_deinitialize();
        return false;
    }

    return true;
}

void network_tick(EngineContext *engine_context) {
    World *world = &engine_context->world;
    ENetHost *host =
            rman_get(&engine_context->rman, RESOURCE_ID(host_enet));
    assert(host);

    SyncPacket packet = create_sync_packet(world);
    if (vector_size(packet.headers) > 0) {
        vector(uint8_t) buffer;
        vector_init(buffer);
        vector_append_multiple(
                buffer, (uint8_t *)&packet.seq_num, sizeof(packet.seq_num));
        for (size_t i = 0; i < vector_size(packet.data); i++) {
            vector_append_multiple(
                    buffer, packet.data[i], vector_size(packet.data[i]));
        }

        ENetPacket *enet_packet = enet_packet_create(
                buffer, vector_size(buffer), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(host, 0, enet_packet);
        vector_destroy(buffer);
    }
    sync_packet_destroy(&packet);

    ENetEvent event;
    while (enet_host_service(host, &event, 1) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            uint8_t *data = event.packet->data;
            size_t len = event.packet->dataLength;
            if (len >= sizeof(uint32_t)) {
                uint32_t seq_num;
                memcpy(&seq_num, data, sizeof(uint32_t));
                deserialize_packet(world,
                        data + sizeof(uint32_t),
                        len - sizeof(uint32_t),
                        seq_num);
            }
            enet_packet_destroy(event.packet);
        }
    }
}

EntityPtr network_create_entity(World *world, Bitmask to_sync) {
    EntityBuilder builder;
    entity_builder_init(&builder, world);

    NetworkId net_id = assign_netid(ENTITY_PTR_INVALID);
    NetworkSync sync = {.to_sync = bitmask_clone(&to_sync),
            .is_synced = false,
            .net_id = net_id};
    entity_builder_add(&builder, COMPONENT_ID(NetworkSync), &sync);

    EntityPtr eptr = world->entity_ptrs[entity_builder_finish(&builder)];
    g_netid_table[net_id] = eptr;
    return eptr;
}
