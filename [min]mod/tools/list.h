#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include "byte.h"
#include "debug.h"

#define LIST_FS "list (length %d)"
#define LIST_FA(list) (int) (list).length

struct List {
        size_t element_size;
        size_t length;
        size_t capacity;
        void * contents;
        void (*element_destructor)(void * element);
};

struct List create_list(size_t element_size, void (*element_destructor)(void * element));

struct List create_invalid_list(void);

bool list_is_valid(const struct List * list);

void * get_list_elem(struct List * list, size_t index);

const void * get_list_elem_const(const struct List * list, size_t index);

void list_append(struct List * list, const void * value);

void list_pop(struct List * list);

void list_remove(struct List * list, size_t index);

void list_insert(struct List * list, size_t index, const void * value);

struct List create_sublist(const struct List * list, size_t begin, size_t end);

#endif
