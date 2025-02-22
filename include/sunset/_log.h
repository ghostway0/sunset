#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "sunset/io.h"
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

typedef void (*LogCallback)(
        LogEvent *ev, Writer *, char const *fmt, va_list args);

void log_set_level(LogLevel level);
void log_set_quiet(bool enable);
void log_add_callback(LogCallback fn, void *udata, LogLevel level);

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

void log_internal(LogLevel level,
        char const *file,
        size_t line,
        char const *fmt,
        ...);
