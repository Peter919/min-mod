// "There should be no more than one class per file, and max 7 functions."
// - someone smart

#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <stdbool.h>
#include "itype.h"

enum StackElemType {
        STACK_ELEM_INVALID,
        STACK_ELEM_INSTR,

        // Napping in a sack
        // Rapping tacky tracks
        // Spitting bars, spitting facts
        // But sud'nly happ'ning happs:
        // The minmod program lacks
        // Stack-elem-sub-stacks
        // So using complex hacks
        // I smartly add them back
        STACK_ELEM_SUBSTACK
};

struct StackElem {
        enum StackElemType type;
        union {
                instr_id_t instr;

                // While variable stacks are passed by reference, literal stacks
                // are always copied by value.
                // It's still a pointer, though, to save some space.
                struct Stack * substack;
        };
        int indirection_level;
};

struct Stack {
        struct StackElem * contents;
        size_t capacity;
        size_t size;
        int reference_count;
};

// Create a new stack without any elements.
// Assumes one variable or stack is referencing the stack at creation.
struct Stack * create_stack(void);

// Create an invalid and unusable stack.
struct Stack * create_invalid_stack(void);

// Returns "true" if "stack" was created using "create_stack" rather than
// "create_invalid_stack".
bool is_stack_valid(const struct Stack * stack);


void add_stack_reference(struct Stack * stack);

void remove_stack_reference(struct Stack * stack);

void destroy_stack(struct Stack * stack);

void destroy_stack_void_ptr(void * stack);

void stack_push(struct Stack * stack, const struct StackElem * stack_elem);

void stack_pop(struct Stack * stack);

struct StackElem * stack_peek(struct Stack * stack, int idx);

// Creates a duplicate of "stack", including deeply copying the sub-stacks.
// It's completely independent, in other words. Like the U. S.
struct Stack * deepcopy_stack(const struct Stack * stack);

// Reverses the contents of "stack". Sub-stacks won't be reversed.
void reverse_stack(struct Stack * stack);

// The three routines below create stack elements from different types of
// data, ready to be sealed and shipped (id est, added to a stack).

struct StackElem instr_to_stack_elem(instr_id_t instr, int indirection_level);

struct StackElem stack_to_stack_elem(struct Stack * substack, int indirection_level);

struct StackElem create_invalid_stack_elem(void);

// Returns "true" if "stack_elem" wasn't created by "create_invalid_stack_elem".
bool is_stack_elem_valid(const struct StackElem * stack_elem);

// Logging stacks backwards is quite useful since instruction
// stacks are reversed before execution
void log_stack_backwards(int log_level, const struct Stack * stack, const struct List * itype_list);

#endif
