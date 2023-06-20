// Implementation of logging tools.

#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

// The string printed before logging a line.
static const char * log_lvl_as_str(int log_level)
{
        switch (log_level) {
        case LOG_LVL_INVALID:
                break;
        case LOG_LVL_DEBUG:
                return "DEBUG";
        case LOG_LVL_INFO:
                return "INFO";
        case LOG_LVL_WARNING:
                return "WARNING";
        case LOG_LVL_ERROR:
                return "ERROR";
        case LOG_LVL_FATAL_ERROR:
                return "FATAL ERROR";
        }
        // Cannot return a string containing "log_level" as an integer,
        // because then the string would have to be dynamically allocated
        // since it's not a literal, and the caller of this function would
        // have to free the allocation and it's just a whole lot of work.
        return "(invalid log level)";
}

static void validate_log_level(int log_level)
{
        switch (log_level) {
        case LOG_LVL_INVALID:
                break;
        case LOG_LVL_DEBUG:
        case LOG_LVL_INFO:
        case LOG_LVL_WARNING:
        case LOG_LVL_ERROR:
        case LOG_LVL_FATAL_ERROR:
        case LOG_LVL_CONSOLE:
                return;
        }

        ASSERT(false, "Invalid log level %d.", (int) log_level);
}

static void validate_log_mode(enum x_LogLvlVisibility log_mode)
{
        switch (log_mode) {
        case LOG_VISIBILITY_INVALID:
                break;
        case LOG_VISIBILITY_SHOW_LEVEL:
        case LOG_VISIBILITY_HIDE_LEVEL:
                return;
        }

        ASSERT(false, "Invalid log mode %d.", (int) log_mode);
}

// Now we can output to any file we want!
// To log directly to the console, make this function return "stdout".
static FILE * get_output_file(void)
{
        return stdout;

        // Uncomment to log to a separate file.
        /*
        static const char * fname = "log.txt";
        FILE * file = fopen(fname, "w");
        if (!file) {
                printf("Could not open \"%s\".\n", fname);
        }
        return file;
        */
}

void x_log(int log_level, enum x_LogLvlVisibility log_mode, const char * message, ...)
{
        static int s_last_log_level = LOG_LVL_INVALID;
        static FILE * output_file = NULL;
        if (!output_file) {
                output_file = get_output_file();
        }

        if (!LOGGABLE(log_level)) {
                return;
        }

        if (log_level == LOG_LVL_CONSOLE) {
                va_list fmt_args;

                va_start(fmt_args, message);
                vprintf(message, fmt_args);
                va_end(fmt_args);

                return;
        }

        validate_log_level(log_level);
        validate_log_mode(log_mode);

        // If we're logging with the same level as before, we don't print
        // the level again.
        if (log_mode == LOG_VISIBILITY_SHOW_LEVEL && log_level != s_last_log_level) {
                fprintf(output_file, "%s: ", log_lvl_as_str(log_level));

                // This variable should only be set if the log level is actually
                // logged. If we log "A" with "LOG_INFO", a newline with invisible
                // "LOG_DEBUG" and then "B" with "LOG_DEBUG", we want
                // "INFO: A
                // DEBUG: B"
                // not
                // "INFO: A
                // B"
                s_last_log_level = log_level;
        }

        va_list fmt_args;

        // Format "message" using "fmt_args" and print the result to "output_file".
        va_start(fmt_args, message);
        vfprintf(output_file, message, fmt_args);
        va_end(fmt_args);
}
