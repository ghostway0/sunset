#include <log.h>

#include "sunset/errors.h"

void error_print(char const *scope, int retval) {
    enum error err = -retval;

    switch (err) {
        case ERROR_IO:
            log_error("%s: io error\n", scope);
            break;
        case ERROR_PARSE:
            log_error("%s: parse error\n", scope);
            break;
        case ERROR_RINGBUFFER_PTR_OVERRUN:
            log_error("%s: ringbuffer ptr overrun\n", scope);
            break;
        default:
            log_error("%s: unknown error\n", scope);
            break;
    }
}
