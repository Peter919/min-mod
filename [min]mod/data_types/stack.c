#include "stack.h"
#include "../tools/mem_tools.h"
#include "../settings.h"

#define MIN_STACK_CAPACITY 2
#define STACK_CAPACITY_MULTIPLIER 1.5

struct Stack * create_stack(void)
{
        struct Stack * stack = ALLOC(struct Stack, 1);
        stack->capacity = MIN_STACK_CAPACITY;
        stack->contents = ALLOC(struct StackElem, stack->capacity);
        stack->size = 0;
        stack->reference_count = 1;

        return stack;
}

struct Stack * create_invalid_stack(void)
{
        // As per the time of writing this comment, "is_stack_valid" simply
        // checks if the capacity is too little for the size or for
        // "MIN_STACK_CAPACITY". Yet, all the variables are initialized
        // with values as bad as possible to make sure that even if
        // "is_stack_valid" for some magical reason changes, it'll still
        // return "false" for stacks created by this function.
        struct Stack * stack = ALLOC(struct Stack, 1);
        stack->capacity = 0;
        stack->contents = NULL;
        stack->size = 1;
        stack->reference_count = -1;

        return stack;
}

// Do not change this function without making sure it will still return
// "false" for values created using "create_invalid_stack".
bool is_stack_valid(const struct Stack * stack)
{
        return stack->capacity >= stack->size && stack->capacity >= MIN_STACK_CAPACITY;
}

void add_stack_reference(struct Stack * stack)
{
        ++stack->reference_count;
}

void remove_stack_reference(struct Stack * stack)
{
        --stack->reference_count;
        if (stack->reference_count == 0) {
                destroy_stack(stack);
        }
}

static void destroy_stack_elem(struct StackElem * elem)
{
        if (elem->type == STACK_ELEM_SUBSTACK) {
                remove_stack_reference(elem->substack);
        }
}

void destroy_stack(struct Stack * stack)
{
        for (size_t i = 0; i < stack->size; ++i) {
                destroy_stack_elem(&stack->contents[i]);
        }

        FREE(stack->contents);
        FREE(stack);
}

void destroy_stack_void_ptr(void * stack)
{
        destroy_stack(stack);
}

static void resize_stack(struct Stack * stack, size_t new_size)
{
        if (new_size < stack->size) {
                for (size_t i = new_size; i < stack->size; ++i) {
                        destroy_stack_elem(&stack->contents[i]);
                }
        }

        while (new_size > stack->capacity) {
                stack->capacity *= STACK_CAPACITY_MULTIPLIER;
        }
        REALLOC(&stack->contents, struct StackElem, stack->capacity);

        stack->size = new_size;
}

void stack_push(struct Stack * stack, const struct StackElem * stack_elem)
{
        resize_stack(stack, stack->size + 1);
        stack->contents[stack->size - 1] = *stack_elem;

        if (stack_elem->type == STACK_ELEM_SUBSTACK) {
                add_stack_reference(stack_elem->substack);
        }
}

void stack_pop(struct Stack * stack)
{
        destroy_stack_elem(&stack->contents[stack->size - 1]);
        resize_stack(stack, stack->size - 1);
}

struct StackElem * stack_peek(struct Stack * stack, int idx)
{
        return &stack->contents[stack->size - idx - 1];
}

struct Stack * deepcopy_stack(const struct Stack * stack)
{
        struct Stack * clone = ALLOC(struct Stack, 1);
        clone->reference_count = 1;
        clone->capacity = stack->capacity;
        clone->size = stack->size;

        clone->contents = ALLOC(struct StackElem, clone->capacity);
        COPY_MEMORY(clone->contents, stack->contents, struct StackElem, clone->capacity);

        // Clone all the sub-stacks.
        for (size_t i = 0; i < clone->size; ++i) {
                struct StackElem * clone_elem = &clone->contents[i];
                const struct StackElem * original_elem = &stack->contents[i];

                if (clone_elem->type == STACK_ELEM_SUBSTACK) {
                        clone_elem->substack = deepcopy_stack(original_elem->substack);
                }
        }

        return clone;
}

void reverse_stack(struct Stack * stack)
{
        int lower_idx = 0;
        int upper_idx = stack->size - 1;

        while (lower_idx < upper_idx) {

                struct StackElem temp = stack->contents[lower_idx];
                stack->contents[lower_idx] = stack->contents[upper_idx];
                stack->contents[upper_idx] = temp;

                ++lower_idx;
                --upper_idx;
        }
}

struct StackElem instr_to_stack_elem(instr_id_t instr, int indirection_level)
{
        struct StackElem stack_elem;
        stack_elem.type = STACK_ELEM_INSTR;
        stack_elem.instr = instr;
        stack_elem.indirection_level = indirection_level;

        return stack_elem;
}

struct StackElem create_stack_ref(struct Stack * stack, int indirection_level)
{
        struct StackElem stack_elem;
        stack_elem.type = STACK_ELEM_STACK_REF;
        stack_elem.indirection_level = indirection_level;

        stack_elem.stack_ref = stack;

        add_stack_reference(stack);

        return stack_elem;
}

struct StackElem create_substack(struct Stack * substack, int indirection_level)
{
        struct StackElem stack_elem;
        stack_elem.type = STACK_ELEM_SUBSTACK;
        stack_elem.indirection_level = indirection_level;

        stack_elem.substack = substack;

        add_stack_reference(substack);

        return stack_elem;
}

struct StackElem create_invalid_stack_elem(void)
{
        struct StackElem stack_elem;
        stack_elem.type = STACK_ELEM_INVALID;
        return stack_elem;
}

bool is_stack_elem_valid(const struct StackElem * stack_elem)
{
        return stack_elem->type != STACK_ELEM_INVALID;
}

static void log_n_times(int log_level, char ch, int count)
{
        for (int i = 0; i < count; ++i) {
                LOG(log_level, "%c", ch);
        }
}

static void log_stack_elem(int log_level,
                           const struct StackElem * stack_elem,
                           const struct List * itype_list)
{
        switch (stack_elem->type) {
        case STACK_ELEM_INSTR: {
                if (is_builtin(stack_elem->instr)) {
                        enum Builtin builtin = id_to_builtin(stack_elem->instr);
                        LOG(log_level, "(%s)", g_builtin_names[builtin]);
                } else {
                        const struct IType * itype;
                        itype = id_to_itype_const(itype_list, stack_elem->instr);

                        LOG(log_level, "%s", itype->name);
                }
                break;
        case STACK_ELEM_STACK_REF:
                LOG(log_level, "&");
                log_stack_backwards(log_level, stack_elem->stack_ref, itype_list);
                break;
        case STACK_ELEM_SUBSTACK:
                log_stack_backwards(log_level, stack_elem->substack, itype_list);
                break;
        default:
                LOG(log_level, "(invalid stack element)");
        }
        }

        log_n_times(log_level, g_indirection_ch, stack_elem->indirection_level);
}

void log_stack_backwards(int log_level, const struct Stack * stack, const struct List * itype_list)
{
        if (!LOGGABLE(log_level)) {
                return;
        }

        LOG(log_level, "%c", g_stack_open_ch);
        for (int i = stack->size - 1; i >= 0; --i) {

                const struct StackElem * curr_elem = &stack->contents[i];
                log_stack_elem(log_level, curr_elem, itype_list);

                if (i != 0) {
                        LOG(log_level, " ");
                }
        }
        LOG(log_level, "%c", g_stack_close_ch);
}
