#pragma once

#include <stdint.h>

#define GLB_MAGIC "0x46546C67"
#define GLB_VERSION 2

struct glb_header {
    char magic[4];
    uint32_t version;
    uint32_t length;
};

struct glb_chunk {
    uint32_t length;
    uint32_t type;
    uint8_t *data;
};
