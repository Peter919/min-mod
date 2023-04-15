// This file is quite poorly written.

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "byte.h"
#include "log.h"

#define ALLOCATIONS_CAPACITY_MULTIPLIER 2

#define MAX_TYPE_NAME_LENGTH 25
#define MAX_FILE_NAME_LENGTH 20

// The amount of allocations logged next to each other.
#define ALLOCATIONS_LOGGED_PER_LINE 3

// The maximum amount of digits assumed to be used by indices, lines and
// sizes of allocations.
#define MAX_ALLOC_IDX_DIGIT_COUNT 6
#define MAX_LINE_DIGIT_COUNT 6
#define MAX_ALLOCATION_SIZE_DIGITS_COUNT 6

// Information about a single allocation.
struct Allocation {
        void * ptr;
        const char * file_name;
        int line;
        const char * type_name;
        size_t type_size;
        size_t count;
};

// Global list of allocation structures.
struct {
        struct Allocation * contents;
        size_t len;
        size_t capacity;
} g_allocs;

bool g_allocs_initialized = false;

static size_t min_valid_allocations_capacity(size_t req_capacity)
{
        size_t capacity = 1;
        while (capacity < req_capacity) {
                capacity *= ALLOCATIONS_CAPACITY_MULTIPLIER;
        }
        return capacity;
}

static void initialize_allocations(void)
{
        ASSERT(!g_allocs_initialized, "Allocations already initialized.");

        g_allocs.len = 0;
        g_allocs.capacity = min_valid_allocations_capacity(0);
        g_allocs.contents = calloc(g_allocs.capacity, sizeof(struct Allocation));
        g_allocs_initialized = true;
}

static struct Allocation * append_allocation(void)
{
        ++g_allocs.len;
        if (g_allocs.len > g_allocs.capacity) {
                g_allocs.capacity = min_valid_allocations_capacity(g_allocs.len);
                g_allocs.contents = realloc(g_allocs.contents,
                        sizeof(struct Allocation) * g_allocs.capacity);
        }

        return &g_allocs.contents[g_allocs.len - 1];
}

static void pop_allocation_index(size_t idx)
{
        // Move everything beyond "idx" one step back, so the allocation at
        // "idx" + 1 is moved to "idx", "idx" + 2 to "idx" + 1 and so on.
        struct Allocation * alloc_at_idx = &g_allocs.contents[idx];
        size_t len_after_idx = g_allocs.len - (idx + 1);
        memmove(alloc_at_idx, alloc_at_idx + 1, len_after_idx * sizeof(struct Allocation));

        // Now that the last part of the list is moved one step back, the
        // last element will have a copy in the second-last index. By
        // decreasing the length, we keep the second copy but get rid of
        // the initial one.
        --g_allocs.len;

        // Resize if capacity is far too large for length.
        if (g_allocs.len * ALLOCATIONS_CAPACITY_MULTIPLIER <= g_allocs.capacity) {
                g_allocs.capacity = min_valid_allocations_capacity(g_allocs.len);
                g_allocs.contents = realloc(g_allocs.contents,
                        sizeof(struct Allocation) * g_allocs.capacity);
        }
}

static const char * file_name_without_path(const char * file_path)
{
        // Some operating systems use slashes to split file paths, so we
        // look for  the last slash and split it there.
        const char * last_slash = strrchr(file_path, '/');
        if (last_slash) {
                // Add 1 to avoid including the slash.
                return last_slash + 1;
        }

        // Other operating systems used backslashes, so we try that too.
        const char * last_backslash = strrchr(file_path, '\\');
        if (last_backslash) {
                // Add 1 to avoid including the backslash.
                return last_backslash + 1;
        }

        // If the file name doesn't have a slash or a backslash, it's safe
        // to assume that its path is simply its name.
        return file_path;
}

// The number of argument should generally be 3 or less, but then
// this function isn't supposed to be called anyways, and if that
// rule is broken the caller deserves a pita.
void * x_allocate(
        const char * file_name,
        int line,
        const char * type_name,
        size_t type_size,
        size_t count)
{
        if (!g_allocs_initialized) {
                initialize_allocations();
        }

        void * allocated_memory = calloc(count, type_size);
        // When allocating 0 bytes, the result might be a NULL pointer.
        // Such pointers are not added to the allocation list, since
        // they may be generated multiple times and we'll have no idea
        // which one we're freeing (if any) when using FREE(NULL);
        if (allocated_memory == NULL && type_size * count == 0) {
                return allocated_memory;
        }

        ASSERT(allocated_memory,
                "Failed to allocate %d instances of \"%s\" in \"%s\", line %d.",
                (int) count, type_name, file_name, line);

        struct Allocation * allocation = append_allocation();
        *allocation = (struct Allocation) {
                .ptr = allocated_memory,
                .file_name = file_name_without_path(file_name),
                .line = line,
                .type_name = type_name,
                .type_size = type_size,
                .count = count
        };

        return allocated_memory;
}

void x_free(void * ptr, const char * file_name, int line)
{
        // Real "free" ignores NULL pointers, so we do to.
        if (ptr == NULL) {
                return;
        }

        bool allocation_found = false;
        // Unused if assertions are disabled; this line prevents a warning.
        (void) allocation_found;
        for (size_t i = 0; i < g_allocs.len; ++i) {
                if (g_allocs.contents[i].ptr == ptr) {
                        pop_allocation_index(i);
                        allocation_found = true;
                        break;
                }
        }
        ASSERT(allocation_found, "%p not allocated in \"%s\", line %d.",
                ptr, file_name, line);
        free(ptr);
}

void * x_realloc(
        void * ptr,
        const char * file_name,
        int line,
        const char * type_name,
        size_t type_size,
        size_t count)
{
        struct Allocation * allocation = NULL;
        int allocation_idx = -1;
        for (size_t i = 0; i < g_allocs.len; ++i) {
                if (g_allocs.contents[i].ptr == ptr) {
                        allocation = &g_allocs.contents[i];
                        allocation_idx = i;
                }
        }

        ASSERT(allocation || ptr == NULL, "%p not allocated in \"%s\", line %d.",
                ptr, file_name, line);

        if (allocation == NULL) {
                allocation = append_allocation();
                allocation_idx = g_allocs.len - 1;
        }

        void * new_ptr = realloc(ptr, type_size * count);

        // "realloc(x, 0)" may return NULL, and in that case we don't want
        // the result in the allocation list since we would never know what
        // NULL pointer to remove (if any) when freeing NULL.
        if (new_ptr == NULL && type_size * count == 0) {
                pop_allocation_index(allocation_idx);
                return NULL;
        }

        ASSERT(new_ptr, "Failed to allocate %d instances of \"%s\" in \"%s\", line %d.",
                (int) count, type_name, file_name, line);

        *allocation = (struct Allocation) {
                .ptr = new_ptr,
                .file_name = file_name_without_path(file_name),
                .line = line,
                .type_name = type_name,
                .type_size = type_size,
                .count = count
        };
        return new_ptr;
}

#ifdef ASSERTIONS
static bool memory_overlaps(const void * mem1, const void * mem2, size_t mem_len)
{
        const void * mem2_first = mem2;
        const void * mem2_last = (byte_t *) mem2 + mem_len - 1;

        for (size_t i = 0; i < mem_len; ++i) {
                void * mem1_at_idx = (byte_t *) mem1 + i;
                if (mem1_at_idx == mem2_first || mem1_at_idx == mem2_last) {
                        return true;
                }
        }
        return false;
}
#endif

void x_copy_memory(void * dest, const void * src, size_t len, const char * file_name, int line)
{
        ASSERT(!memory_overlaps(dest, src, len),
                "Cannot copy overlapping memory in \"%s\", line %d.",
                file_name, line);

        memcpy(dest, src, len);
}

void x_move_memory(void * dest, const void * src, size_t len)
{
        memmove(dest, src, len);
}

void x_log_allocations(void)
{
        LOG_DEBUG("Allocations (pointer, file, line, type, count):\n");
        for (size_t i = 0; i < g_allocs.len; ++i) {

                // If a full line of allocations has been logged, log a new
                // line. Otherwise, log a "|" separating allocations.
                if (i % ALLOCATIONS_LOGGED_PER_LINE == 0 && i > 0) {
                        LOG_DEBUG("\n");
                } else if (i > 0) {
                        LOG_DEBUG(" | ");
                }

                LOG_DEBUG("%*d ", MAX_ALLOC_IDX_DIGIT_COUNT, i);

                LOG_DEBUG("%p ", g_allocs.contents[i].ptr);

                LOG_DEBUG("%-*.*s ", MAX_FILE_NAME_LENGTH, MAX_FILE_NAME_LENGTH,
                        g_allocs.contents[i].file_name);

                LOG_DEBUG("%*d ", MAX_LINE_DIGIT_COUNT, g_allocs.contents[i].line);

                LOG_DEBUG("%-*.*s ", MAX_TYPE_NAME_LENGTH, MAX_TYPE_NAME_LENGTH,
                        g_allocs.contents[i].type_name);

                LOG_DEBUG("%*d ", MAX_ALLOCATION_SIZE_DIGITS_COUNT,
                        (int) g_allocs.contents[i].count);
        }
        LOG_DEBUG("\n");
}

bool x_is_allocated(const void * ptr)
{
        // NULL pointers can be freed, so they count as allocated.
        if (!ptr) {
                return true;
        }

        for (size_t i = 0; i < g_allocs.len; ++i) {
                if (g_allocs.contents[i].ptr == ptr) {
                        return true;
                }
        }
        return false;
}

size_t x_mem_in_use(void)
{
        size_t mem = 0;
        for (size_t i = 0; i < g_allocs.len; ++i) {
                mem += g_allocs.contents[i].type_size * g_allocs.contents[i].count;
        }
        return mem;
}
