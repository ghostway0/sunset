#pragma once

enum error {
    ERROR_IO = 1,
    ERROR_PARSE,
    ERROR_RINGBUFFER_PTR_OVERRUN,
};

void error_print(const char *scope, int retval);
