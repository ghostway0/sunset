#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "internal/utils.h"
#include "sunset/vector.h"

ssize_t vector_writeback(void *ctx, void const *buf, size_t count) {
    VectorWriterContext *vctx = ctx;

    size_t num_elements = count / vctx->element_size;

    if (count % vctx->element_size != 0) {
        return -1;
    }

    size_t prev_size = vector_size(vctx->vector);

    vector_resize2(
            vctx->vector, (prev_size + num_elements) * vctx->element_size);

    write(1, buf, count);

    memcpy(vctx->vector + prev_size, buf, count);

    return count;
}
