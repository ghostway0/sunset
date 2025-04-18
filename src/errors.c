#include <log.h>

#include "sunset/errors.h"

void error_print(char const *scope, int retval) {
    Error err = -retval;

    switch (err) {
        case ERROR_IO:
            log_error("%s: io error\n", scope);
            break;
        case ERROR_INVALID_FORMAT:
            log_error("%s: parse error\n", scope);
            break;
        default:
            log_error("%s: unknown error\n", scope);
            break;
    }
}
