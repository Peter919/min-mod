// Tools for allocating and freeing memory, in addition to a couple
// of other memory-related functions.

#ifndef MEM_TOOLS_H
#define MEM_TOOLS_H
#include <string.h>
#include "debug.h"

// If DEBUG_ON, use safe but slow memory functions. Otherwise, use
// the fast but dangerous ones.
#if DEBUG_ON

        // Allocate "count" instances of type "type"
        #define ALLOC(type, count) \
                (type *) x_allocate(__FILE__, __LINE__, #type, sizeof(type), count)

        // Free "ptr". Pointer must be "ALLOC"'d or equal to "NULL".
        #define FREE(ptr) x_free(ptr, __FILE__, __LINE__)

        // Assign "ptr", assumed to be allocated using "ALLOC", to
        // "count" instances of type "type" and free its previous
        // contents. A "REALLOC"'d pointer still counts as allocated
        // with "ALLOC".
        #define REALLOC(ptr, type, count) \
                (void) (*(ptr) = x_realloc(*(ptr), __FILE__, __LINE__, #type, sizeof(type), count))

        // Shallowly copy "count" instances of type "type" from "src"
        // to "dest". "src" and "dest" cannot have overlapping memory.
        #define COPY_MEMORY(dest, src, type, count) \
                x_copy_memory(dest, src, sizeof(type) * (count), __FILE__, __LINE__)

        // Like "COPY_MEMORY", except it's safe to copy overlapping memory.
        #define MOVE_MEMORY(dest, src, type, count) \
                x_move_memory(dest, src, sizeof(type) * (count))

        // Log everything that's allocated using "ALLOC" and not yet
        // freed using "FREE".
        #define LOG_ALLOCATIONS() x_log_allocations()

        // Returns "true" if "ptr" is allocated using "ALLOC".
        #define IS_ALLOCATED(ptr) x_is_allocated(ptr)

        // Returns the number of bytes allocated using "ALLOC".
        #define MEM_IN_USE() x_mem_in_use()

#else

        // Ya basic!
        // It's a human insult.
        // It's devastating.
        // You're devastated right now.
        // These macros does, in fact, use basic and really unsafe
        // routines. "DEBUG_MODE" is basically a choice between
        // a costly error checking and even more costly debugging,
        // which is exactly what we want since choices without
        // risks are not interesting choices.
        #define ALLOC(type, count) (type *) malloc(sizeof(type) * (count))
        #define FREE(ptr) free(ptr)
        #define REALLOC(ptr, type, count) \
                (void) (*(ptr) = realloc(*(ptr), sizeof(type) * (count)))
        #define COPY_MEMORY(dest, src, type, count) memcpy(dest, src, sizeof(type) * (count))
        #define MOVE_MEMORY(dest, src, type, count) memmove(dest, src, sizeof(type) * (count))
        #define LOG_ALLOCATIONS(log_level)

        // "MEM_IN_USE" and "IS_ALLOCATED" not defined since it's only accessible
        // when using special memory macros.

#endif

#define SET_MEMORY(dest, val, type, count) memset(dest, val, sizeof(type) * count)
#define MEMORY_EQUALS(mem1, mem2, type, count) (memcmp(mem1, mem2, sizeof(type) * count) == 0)

void * x_allocate(
        const char * file_name,
        int line,
        const char * type_name,
        size_t type_size,
        size_t count);

void x_free(void * ptr, const char * file_name, int line);

void * x_realloc(
        void * ptr,
        const char * file_name,
        int line,
        const char * type_name,
        size_t type_size,
        size_t count);

void x_copy_memory(void * dest, const void * src, size_t len, const char * file_name, int line);

void x_move_memory(void * dest, const void * src, size_t len);

void x_log_allocations(void);

bool x_is_allocated(const void * ptr);

size_t x_mem_in_use(void);

#endif
