#pragma once

enum error {
    ERROR_IO = 1,
    ERROR_PARSE,
    ERROR_RINGBUFFER_PTR_OVERRUN,
    ERROR_INVALID_ARGUMENTS,
    ERROR_STREAM_OVERRUN,
};

void error_print(char const *scope, int retval);
