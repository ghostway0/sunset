#pragma once

#define EPSILON 0.001

#define unused(...) \
    __VA_ARGS_FOR_EACH(__unused_helper, __VA_ARGS__)

#define __unused_helper(x) ((void)(x))

#define __VA_ARGS_FOR_EACH(macro, ...) \
    __VA_ARGS_FOR_EACH_IMPL(macro, __VA_ARGS__)

#define __VA_ARGS_FOR_EACH_IMPL(macro, first, ...) \
    macro(first) __VA_OPT__(__VA_ARGS_FOR_EACH_STEP(macro, __VA_ARGS__))

#define __VA_ARGS_FOR_EACH_STEP(macro, first, ...) \
    , macro(first) __VA_OPT__(__VA_ARGS_FOR_EACH_STEP(macro, __VA_ARGS__))
