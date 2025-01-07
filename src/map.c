#include "sunset/map.h"

size_t map_find_index(void const *v,
        size_t size,
        size_t elem_size,
        void const *value,
        Order (*compar)(void const *, void const *)) {
    size_t low = 0;
    size_t high = size;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        int cmp = compar((char *)v + mid * elem_size, value);
        if (cmp < 0) {
            low = mid + 1;
        } else if (cmp > 0) {
            high = mid;
        } else {
            return mid;
        }
    }

    return low;
}
