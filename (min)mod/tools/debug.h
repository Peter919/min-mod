// Tools for debugging and error handling, used in pcecs.

#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include <time.h>

// The compiler isn't able to correctly check for equality between
// compile-time constants and enumerated constants, so these are
// "#define"d.
// None are 1 because then that value would correctly compare to
// "DEBUG_MODE" if "DEBUG_MODE" was defined as nothing.
#define DEBUG_MODE_OFF 0
#define DEBUG_MODE_ON 2

#define DEBUG_OFF (DEBUG_MODE == DEBUG_MODE_OFF)
#define DEBUG_ON (DEBUG_MODE == DEBUG_MODE_ON)

// Choose global debug mode here!
// Each individual file might define "DEBUG_MODE" before directly
// or indirectly including this file to have a different debug mode
// than the default one.
#ifndef DEBUG_MODE
        #ifdef NDEBUG
                #define DEBUG_MODE DEBUG_MODE_OFF
        #else
                #define DEBUG_MODE DEBUG_MODE_ON
        #endif
#endif

#if DEBUG_ON

        #define ASSERTIONS

        #define TIME_CODE(code) do { \
                clock_t start = clock(); \
                /* Making sure "code" is in curly brackets, because that's
                 * a convention I use. */ \
                do code while(0); \
                clock_t end = clock(); \
                \
                double time_used = ((double) (end - start)) / CLOCKS_PER_SEC; \
                LOG_DEBUG("\"" #code "\" took %lf seconds to execute.\n", time_used); \
        } while (0)

        #define DEBUG_CODE(code) do code while(0)

#elif DEBUG_OFF
        #define TIME(code) do code while(0)
        #define DEBUG_CODE(code)
#else
        #error "Invalid debug mode."
#endif

#ifdef ASSERTIONS
        #define ASSERT(cond, msg, ...) do { \
                if (!(cond)) { \
                        /* Print the function, line and file before printing "msg", so
                         * that the important stuff will be printed even if the format
                         * arguments crashes the program. */ \
                        LOG_FATAL_ERROR( \
                                "Assertion %s failed in function %s, " \
                                "line %d in file \"%s\": ", \
                                #cond, __func__, __LINE__, __FILE__); \
                        \
                        LOG_FATAL_ERROR(msg __VA_OPT__(,) __VA_ARGS__); \
                        LOG_FATAL_ERROR("\n"); \
                        \
                        proper_exit(EXIT_FAILURE); \
                } \
        } while (0)

        #define ASSERT_OR_HANDLE(cond, err_return_val, msg, ...) \
                ASSERT(cond, msg __VA_OPT__(,) __VA_ARGS__)

        // My compiler does not have static_assert.
        #ifdef __GNUC__

                // Only GNU is cool enough for __attribute((__unused__)).
                #define STATIC_ASSERT(cond) \
                /* Trick I learned from Stack Overflow.
                 * Now i can't find it :(
                 * The array length is -1 (invalid) if "cond" is false. */ \
                __attribute__((__unused__)) extern int x_static_assert_arr[(cond) ? 1 : -1]
        #else
                #define STATIC_ASSERT(cond) \
                __attribute((__maybe_unused__)) extern int x_static_assert_arr[(cond) ? 1 : -1]
        #endif
#else
        #define ASSERT(cond, msg, ...)
        #define ASSERT_OR_HANDLE(cond, err_return_val, msg, ...) do { \
                if (!(cond)) { \
                        LOG_ERROR(msg __VA_OPT__(,) __VA_ARGS__); \
                        /* Assertions don't generally have a newline at the end,
                         * but it's necessary when logging. */\
                        LOG_ERROR("\n"); \
                        return err_return_val; \
                } \
        } while(0)
        #define STATIC_ASSERT(cond)
#endif

// Make a user press a key before exiting.
// When ran directly from Code::Blocks, the console prompts that by default, but
// when ran as an ".exe"-file, it just exits.
void proper_exit(int exit_code);

// "log.h" itself relies on "debug.h". Therefore, we include log.h at the very end to
// assure that all necessary info from this file is already initialized so "log.h"
// can safely be initialized without any missing parts from this file. I admit this
// sort of circular dependency is cumbersome, but at least it's quite loosely
// coupled; this file only needs log for its assertion.
#include "log.h"

#endif
