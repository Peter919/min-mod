// Tools for logging, including a global minimum log level for the
// limit to how unimportant messages we want to log.
// "LOG_MODE" can be defined before including this file directly
// or indirectly to change whether a certain file logs or not.
// Same goes for "MIN_LOG_LEVEL" below.

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include "debug.h"

// They're macros because preprocessor equality doesn't work with
// enums. Stupid ANSI or Brian or Dennis or whomever.
#define LOG_LVL_INVALID 0
#define LOG_LVL_DEBUG 1
#define LOG_LVL_INFO 2
#define LOG_LVL_WARNING 3
#define LOG_LVL_ERROR 4
#define LOG_LVL_FATAL_ERROR 5
#define LOG_LVL_CONSOLE 6
#define LOG_LVL_NONE 7

#ifndef MIN_LOG_LEVEL
        #if DEBUG_ON
                #define MIN_LOG_LEVEL LOG_LVL_DEBUG
        #else
                #define MIN_LOG_LEVEL LOG_LVL_WARNING
        #endif
#endif

// If this was a function and not a macro, we couldn't redefine
// "LOG_MODE" and "MIN_LOG_LEVEL" in specific files, since the
// same source file would do the logging anyway.
#define LOG(log_level, message, ...) \
                x_log(log_level, LOG_VISIBILITY_SHOW_LEVEL, message __VA_OPT__(,) __VA_ARGS__)

// Log without showing the level at the beginning of the string.
#define LOG_HIDE_LEVEL(log_level, message, ...) \
                x_log(log_level, LOG_VISIBILITY_HIDE_LEVEL, message __VA_OPT__(,) __VA_ARGS__)

#define LOGGABLE(lvl) ((lvl) >= MIN_LOG_LEVEL)

// What a beauty ...
#if LOGGABLE(LOG_LVL_DEBUG)
#define LOG_DEBUG(...) LOG(LOG_LVL_DEBUG, __VA_ARGS__)
#define LOG_DEBUG_HIDE_LEVEL(...) LOG_HIDE_LEVEL(LOG_LVL_DEBUG, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#define LOG_DEBUG_HIDE_LEVEL(...)
#endif

#if LOGGABLE(LOG_LVL_INFO)
#define LOG_INFO(...) LOG(LOG_LVL_INFO, __VA_ARGS__)
#else
#define LOG_INFO(...)
#endif

#if LOGGABLE(LOG_LVL_WARNING)
#define LOG_WARNING(...) LOG(LOG_LVL_WARNING, __VA_ARGS__)
#else
#define LOG_WARNING(...)
#endif

#if LOGGABLE(LOG_LVL_ERROR)
#define LOG_ERROR(...) LOG(LOG_LVL_ERROR, __VA_ARGS__)
#else
#define LOG_ERROR(...)
#endif

#if LOGGABLE(LOG_LVL_FATAL_ERROR)
#define LOG_FATAL_ERROR(...) LOG(LOG_LVL_FATAL_ERROR, __VA_ARGS__)
#else
#define LOG_FATAL_ERROR(...)
#endif

enum x_LogLvlVisibility
{
        LOG_VISIBILITY_INVALID,
        LOG_VISIBILITY_SHOW_LEVEL,
        LOG_VISIBILITY_HIDE_LEVEL
};

void x_log(int log_level, enum x_LogLvlVisibility log_mode, const char * message, ...);

#endif
