#include <stdbool.h>
#include <stddef.h>

#include <assert.h>

#include "sunset/vector.h"

#define MAX_NUM_CALLBACKS 8

enum LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    NUM_LEVELS,
} typedef LogLevel;

typedef struct {
    char const *file;
    struct tm *time;
    void *udata;
    size_t line;
    LogLevel level;
} LogEvent;

typedef void (*LogCallback)(LogEvent *ev, char const *fmt, ...);

char const *log_level_string(int level);
void log_set_level(LogLevel level);
void log_set_quiet(bool enable);
int log_add_callback(LogCallback fn, void *udata, int level);

struct Logger {
    vector(LogCallback) callbacks[NUM_LEVELS];
    vector(void *) udatas[NUM_LEVELS];
    LogLevel level;
    bool quiet;
} typedef Logger;

#define log_trace(...)                                                     \
    log_internal(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...)                                                     \
    log_internal(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)                                                      \
    log_internal(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)                                                      \
    log_internal(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)                                                     \
    log_internal(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...)                                                     \
    log_internal(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

static Logger logger;

void log_add_callback(LogCallback fn, void *udata, int level) {
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
        for (int i = 0; i < NUM_LEVELS; i++) {
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
    for (int i = 0; i < vector_size(logger.callbacks[level]); i++) {
        Writer *writer = (Writer *)logger.udatas[level][i];
        va_start(args, fmt);
        logger.callbacks[level][i](&ev, writer, fmt, args);
        va_end(args);
    }

    if (level == LOG_FATAL) {
        abort();
    }
}

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
    writer_printf(writer,
            "%s %-5s %s:%zu: ",
            buf,
            log_level_string(ev->level),
            ev->file,
            ev->line);
#endif

    writer_vprintf(writer, fmt, args);
    writer_write(writer, "\n", 1);
}
