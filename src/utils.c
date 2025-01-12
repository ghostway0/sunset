#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "internal/time_utils.h"

float get_time_s() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec / 1e9;
}

uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

struct timespec get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

uint64_t time_since_ms(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000
            + (now.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t time_since_us(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000000
            + (now.tv_nsec - start.tv_nsec) / 1000;
}

float time_since_s(struct timespec start) {
    struct timespec now = get_time();
    return (float)(now.tv_sec - start.tv_sec)
            + (float)(now.tv_nsec - start.tv_nsec) / 1000000000.0;
}

void memswap(void *a, void *b, size_t n) {
    uint8_t *mem_a = a;
    uint8_t *mem_b = b;
    uint8_t scratch;

    // let's hope this vectorizes!
    for (size_t i = 0; i < n; i++) {
        scratch = *mem_a;
        *mem_a = *mem_b;
        *mem_b = scratch;
    }
}
