#ifndef MATH_H
#define MATH_H

#define min(a, b) ((a) < (b) ? (a) : (b))

#define max(a, b) ((a) > (b) ? (a) : (b))

#define clamp(x, lower, upper) (min(max((x), (lower)), (upper)))

#define within(x, lower, upper) ((x) >= (lower) && (x) <= (upper))

#define top_percentile(arr, n, p, compare)                                     \
    ({                                                                         \
        size_t *sorted = malloc(n * sizeof(size_t));                           \
        memcpy(sorted, arr, n * sizeof(size_t));                               \
        qsort(sorted, n, sizeof(size_t), compare);                             \
        size_t idx = n - n * p / 100;                                          \
        size_t result = sorted[idx];                                           \
        free(sorted);                                                          \
        result;                                                                \
    })


#endif // MATH_H
