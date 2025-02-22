#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "sunset/io.h"
#include "sunset/vector.h"

#include "sunset/log.h"

static Logger logger;

static char const *log_level_string(LogLevel level) {
    switch (level) {
        case LOG_TRACE:
            return "TRACE";
        case LOG_DEBUG:
            return "DEBUG";
        case LOG_INFO:
            return "INFO";
        case LOG_WARN:
            return "WARN";
        case LOG_ERROR:
            return "ERROR";
        case LOG_FATAL:
            return "FATAL";
        default:
            unreachable();
    }
}

#undef LOG_USE_COLOR

static void log_callback(
        LogEvent *ev, Writer *writer, char const *fmt, va_list args) {
    if (logger.quiet) {
        return;
    }

    time_t t = time(NULL);
    ev->time = localtime(&t);

    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';

#ifdef LOG_USE_COLOR
    writer_printf(writer,
            "%s %s%-5s\x1b[0m \x1b[90m%s:%zu:\x1b[0m ",
            buf,
            level_colors[ev->level],
            log_level_string(ev->level),
            ev->file,
            ev->line);
#else
    writer_vprintf(writer,
            "%s %s %s:%zu: ",
            buf,
            log_level_string(ev->level),
            ev->file,
            ev->line);
#endif

    writer_vprintf(writer, fmt, args);
    writer_write(writer, "\n", 1);
}

void log_add_callback(LogCallback fn, void *udata, LogLevel level) {
    assert(level < NUM_LEVELS);

    if (!logger.callbacks[level]) {
        vector_init(logger.callbacks[level]);
        vector_init(logger.udatas[level]);
    }

    vector_append(logger.callbacks[level], fn);
    vector_append(logger.udatas[level], udata);
}

void log_internal(LogLevel level,
        char const *file,
        size_t line,
        char const *fmt,
        ...) {
    if (level < logger.level) {
        return;
    }

    static bool initialized = false;

    if (!initialized) {
        for (LogLevel i = LOG_TRACE; i < NUM_LEVELS; i++) {
            vector_init(logger.callbacks[i]);
            vector_init(logger.udatas[i]);
        }

        log_add_callback(log_callback, get_stdout(), LOG_TRACE);

        initialized = true;
    }

    LogEvent ev = {
            .file = file,
            .line = line,
            .level = level,
    };

    va_list args;

    for (LogLevel curr = LOG_TRACE; curr < level; curr++) {
        for (size_t i = 0; i < vector_size(logger.callbacks[curr]); i++) {
            Writer *writer = (Writer *)logger.udatas[curr][i];
            va_start(args, fmt);
            logger.callbacks[curr][i](&ev, writer, fmt, args);
            va_end(args);
        }
    }

    if (level == LOG_FATAL) {
        abort();
    }
}

void log_set_level(LogLevel level) {
    logger.level = level;
}
