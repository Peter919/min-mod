#include "list.h"
#include "mem_tools.h"

#define MIN_CAPACITY 2
#define CAPACITY_MULTIPLIER 1.5

// Defined as a macro so that it can be used for both const and non-
// const lists, returning a const or non-const value, respectively.
#define GET_ELEMENT(list, index, dest) do { \
        ASSERT((index) >= 0 && (index) < (list)->length, \
               "Index %d out of list range (0 - %d, inclusive).", \
               index, (list)->length - 1); \
        \
        /* Void pointer arithmetics are apparently unportable. */ \
        *(dest) = (byte_t *) (list)->contents + (index) * (list)->element_size; \
} while (0)

static size_t min_valid_capacity(size_t length)
{
        size_t capacity = MIN_CAPACITY;
        while (capacity < length) {
                capacity *= CAPACITY_MULTIPLIER;
        }
        return capacity;
}

struct List create_list(size_t element_size, void (*element_destructor)(void * element))
{
        struct List list;
        list.element_size = element_size;
        list.length = 0;
        list.capacity = min_valid_capacity(list.length);
        list.contents = ALLOC(byte_t, list.capacity * list.element_size);
        list.element_destructor = element_destructor;

        return list;
}

struct List create_invalid_list(void)
{
        // An element size of 0, and possibly a capacity of 0 as well,
        // depending on the value of "MIN_CAPACITY"; is invalid.
        struct List list;
        list.element_size = 0;
        list.length = 0;
        list.capacity = 0;
        list.contents = NULL;
        list.element_destructor = NULL;

        return list;
}

bool list_is_valid(const struct List * list)
{
        if (list->element_size == 0 || list->capacity < MIN_CAPACITY) {
                return false;
        }
        return true;
}

void * get_list_elem(struct List * list, size_t index)
{
        void * element;
        GET_ELEMENT(list, index, &element);
        return element;
}

const void * get_list_elem_const(const struct List * list, size_t index)
{
        const void * element;
        GET_ELEMENT(list, index, &element);
        return element;
}

// Returns "true" if and only if "list->capacity" isn't large enough,
// or is unnecessarily large, for "new_length".
static bool new_capacity_needed(size_t capacity, size_t length)
{
        bool too_much_capacity = capacity >= length * 2 && capacity != 1;
        bool not_enough_capacity = capacity < length;

        return too_much_capacity || not_enough_capacity;
}

static void change_capacity(struct List * list, size_t new_length)
{
        list->capacity = min_valid_capacity(new_length);
        REALLOC(&list->contents, byte_t, list->capacity * list->element_size);
}

static void set_length(struct List * list, size_t new_length)
{
        if (new_capacity_needed(list->capacity, new_length)) {
                change_capacity(list, new_length);
        }

        list->length = new_length;
}

void list_append(struct List * list, const void * value)
{
        set_length(list, list->length + 1);
        COPY_MEMORY((byte_t *) list->contents + (list->length - 1) * list->element_size,
                    (byte_t *) value, byte_t, list->element_size);
}

void list_pop(struct List * list)
{
        if (list->element_destructor) {
                void * last_elem = get_list_elem(list, list->length - 1);
                list->element_destructor(last_elem);
        }

        set_length(list, list->length - 1);
}

void list_remove(struct List * list, size_t index)
{
        if (index == list->length - 1) {
                list_pop(list);
                return;
        }

        void * element_at_index = get_list_elem(list, index);
        if (list->element_destructor) {
                list->element_destructor(element_at_index);
        }

        void * element_after_index = get_list_elem(list, index + 1);
        size_t elements_to_move = list->length - index - 1;

        MOVE_MEMORY(element_at_index, element_after_index,
                    byte_t, elements_to_move * list->element_size);

        set_length(list, list->length - 1);
}

void list_insert(struct List * list, size_t index, const void * value)
{
        if (index == list->length - 1) {
                list_append(list, value);
                return;
        }

        set_length(list, list->length + 1);

        void * element_at_index = get_list_elem(list, index);
        void * element_after_index = get_list_elem(list, index + 1);
        size_t elements_to_move = list->length - index - 1;

        MOVE_MEMORY(element_after_index, element_at_index,
                    byte_t, elements_to_move * list->element_size);

        COPY_MEMORY(element_at_index, value, byte_t, list->element_size);
}

struct List create_sublist(const struct List * list, size_t begin, size_t end)
{
        struct List sublist = create_list(list->element_size, list->element_destructor);

        // Not the fastest way to do this, but it works.
        for (size_t i = begin; i < end; ++i) {
                const void * curr_elem = get_list_elem_const(list, i);
                list_append(&sublist, curr_elem);
        }

        return sublist;
}
