#ifndef MATH_H
#define MATH_H

#define min(a, b) ((a) < (b) ? (a) : (b))

#define max(a, b) ((a) > (b) ? (a) : (b))

#define clamp(x, lower, upper) (min(max((x), (lower)), (upper)))

#define within(x, lower, upper) ((x) >= (lower) && (x) <= (upper))

#endif // MATH_H
