// This file became a little uglier every time an error was fixed.
// Now, I just have to pray that the errors are all gone.

#include "running.h"
#include <stdio.h>
#include "../settings.h"
#include "../data_types/stack.h"
#include "../tools/mem_tools.h"
#include "../tools/log.h"

typedef enum ErrState (*builtin_func_t)(struct List * itype_list,
                                        instr_id_t data_stack_instr,
                                        instr_id_t instr_stack_instr);

static void pop_from_instr_substack(struct Stack * instr_stack, struct Stack * instr_substack)
{
        if (instr_substack->size == 0) {
                stack_pop(instr_stack);
        } else {
                stack_pop(instr_substack);
        }
}

static enum ErrState set_instr(struct List * itype_list,
                               instr_id_t data_stack_instr,
                               instr_id_t instr_stack_instr)
{
        struct IType * data_stack_itype = get_list_elem(itype_list, data_stack_instr);
        struct Stack * data_stack = data_stack_itype->value;

        ASSERT_OR_HANDLE(data_stack->size >= 2, ERR_FAILURE,
                         "%s instruction requires data stack with 2 elements or more, got %d.",
                         g_builtin_names[0], data_stack->size);

        instr_id_t instr;

        struct StackElem * arg1 = stack_peek(data_stack, 0);

        ASSERT_OR_HANDLE(arg1->type == STACK_ELEM_INSTR, ERR_FAILURE,
                         "First argument of %s instruction must be an instruction.",
                         g_builtin_names[0]);

        ASSERT_OR_HANDLE(arg1->indirection_level == 0, ERR_FAILURE, "First argument of %s "
                         "instruction cannot have any level of indirection.",
                         g_builtin_names[0]);

        ASSERT_OR_HANDLE(!is_builtin(arg1->instr), ERR_FAILURE, "Cannot set a built-in");

        instr = arg1->instr;

        struct StackElem * arg2 = stack_peek(data_stack, 1);

        struct Stack * new_val = NULL;
        if (arg2->type == STACK_ELEM_SUBSTACK) {
                new_val = deepcopy_stack(arg2->substack);
        } else {
                struct IType * itype = get_list_elem(itype_list, arg2->instr);
                new_val = itype->value;

                if (new_val) {
                        add_stack_reference(new_val);
                }
        }

        // Must happen before a stack changes its value in case "data_stack"
        // changes.
        stack_pop(data_stack);
        stack_pop(data_stack);

        struct IType * itype = get_list_elem(itype_list, instr);
        if (itype->value) {
                remove_stack_reference(itype->value);
        }
        itype->value = new_val;

        return ERR_SUCCESS;
}

static enum ErrState move_instr(struct List * itype_list,
                                instr_id_t data_stack_instr,
                                instr_id_t instr_stack_instr)
{
        struct IType * data_stack_itype = get_list_elem(itype_list, data_stack_instr);
        struct Stack * data_stack = data_stack_itype->value;

        ASSERT_OR_HANDLE(data_stack->size >= 2, ERR_FAILURE,
                         "%s instruction requires data stack with 2 elements or more, got %d.",
                         g_builtin_names[1], data_stack->size);

        struct StackElem * arg1 = stack_peek(data_stack, 0);
        struct Stack * dest;
        if (arg1->type == STACK_ELEM_INSTR) {
                ASSERT_OR_HANDLE(!is_builtin(arg1->instr), ERR_FAILURE,
                                 "Cannot move to a built-in.");

                struct IType * itype = get_list_elem(itype_list, arg1->instr);
                dest = itype->value;

                ASSERT_OR_HANDLE(dest, ERR_FAILURE, "Cannot move to uninitialized stack.");
        } else {
                LOG_WARNING("Useless move to stack literal.\n");
                dest = deepcopy_stack(arg1->substack);
        }

        struct StackElem * arg2 = stack_peek(data_stack, 1);
        struct Stack * src;
        if (arg2->type == STACK_ELEM_INSTR) {
                ASSERT_OR_HANDLE(!is_builtin(arg2->instr), ERR_FAILURE,
                                 "Cannot move from a built-in.");

                struct IType * itype = get_list_elem(itype_list, arg2->instr);
                src = itype->value;

                ASSERT_OR_HANDLE(dest, ERR_FAILURE, "Cannot move from uninitialized stack.");
        } else {
                LOG_WARNING("Useless move from stack literal.", g_builtin_names[1]);
                src = deepcopy_stack(arg2->substack);
        }

        ASSERT_OR_HANDLE(src->size > 0, ERR_FAILURE, "Cannot move from empty stack.");

        struct StackElem * src_top = stack_peek(src, 0);
        stack_push(dest, src_top);
        stack_pop(src);

        stack_pop(data_stack);
        stack_pop(data_stack);

        return ERR_FAILURE;
}

static enum ErrState if_instr(struct List * itype_list,
                              instr_id_t data_stack_instr,
                              instr_id_t instr_stack_instr)
{
        struct IType * data_stack_itype = get_list_elem(itype_list, data_stack_instr);
        struct Stack * data_stack = data_stack_itype->value;

        ASSERT_OR_HANDLE(data_stack->size >= 1, ERR_FAILURE,
                         "%s instruction requires data stack with 1 element or more, got %d.",
                         g_builtin_names[2], data_stack->size);

        struct StackElem * arg = stack_peek(data_stack, 0);
        struct Stack * arg_val;
        if (arg->type == STACK_ELEM_INSTR) {

                ASSERT_OR_HANDLE(!is_builtin(arg->instr), ERR_FAILURE,
                                 "Cannot perform if instruction on built-in.");

                struct IType * itype = get_list_elem(itype_list, arg->instr);
                arg_val = itype->value;

                ASSERT(arg_val, "Cannot perform %s instruction on uninitialized stack.",
                       g_builtin_names[2]);
        } else {
                arg_val = arg->substack;
        }

        if (arg_val->size == 0) {
                stack_pop(data_stack);
                stack_pop(data_stack);
        } else {
                stack_pop(data_stack);
        }

        return ERR_SUCCESS;
}

static enum ErrState step(struct List * itype_list,
                          instr_id_t data_stack_instr,
                          instr_id_t instr_stack_instr)
{
        static const builtin_func_t builtin_funcs[] = {
                set_instr,
                move_instr,
                if_instr
        };

        struct IType * data_stack_itype = get_list_elem(itype_list, data_stack_instr);
        struct Stack * data_stack = data_stack_itype->value;

        struct IType * instr_stack_itype = get_list_elem(itype_list, instr_stack_instr);
        struct Stack * instr_stack = instr_stack_itype->value;

        if (instr_stack->size == 0) {
                return ERR_SUCCESS;
        }

        struct StackElem * instr_stack_top = stack_peek(instr_stack, 0);

        ASSERT_OR_HANDLE(instr_stack_top->type == STACK_ELEM_SUBSTACK, ERR_FAILURE,
               "Top value in instruction stack not a sub-stack.");

        struct Stack * instr_substack = instr_stack_top->substack;

        if (instr_substack->size == 0) {
                stack_pop(instr_stack);
                return ERR_UNFINISHED;
        }

        struct StackElem * substack_top = stack_peek(instr_substack, 0);

        if (substack_top->indirection_level > 0) {

                --substack_top->indirection_level;
                stack_push(data_stack, substack_top);
                ++substack_top->indirection_level;

                pop_from_instr_substack(instr_stack, instr_substack);

                return ERR_UNFINISHED;
        }

        if (substack_top->type == STACK_ELEM_SUBSTACK) {

                struct StackElem new_substack = stack_to_stack_elem(substack_top->substack, 0);
                stack_push(instr_stack, &new_substack);

                pop_from_instr_substack(instr_stack, instr_substack);
                return ERR_UNFINISHED;
        }

        if (!is_builtin(substack_top->instr)) {

                struct IType * instr = get_list_elem(itype_list, substack_top->instr);

                ASSERT_OR_HANDLE(instr->value, ERR_FAILURE,
                                 "Cannot execute uninitialized instruction \"%s\".",
                                 instr->name);

                struct Stack * instr_val_copy = deepcopy_stack(instr->value);
                struct StackElem new_substack = stack_to_stack_elem(instr_val_copy, 0);
                stack_push(instr_stack, &new_substack);

                pop_from_instr_substack(instr_stack, instr_substack);

                return ERR_UNFINISHED;
        }

        enum Builtin builtin = id_to_builtin(substack_top->instr);

        pop_from_instr_substack(instr_stack, instr_substack);

        enum ErrState err_state;
        err_state = (builtin_funcs[builtin])(itype_list, data_stack_instr, instr_stack_instr);

        if (err_state == ERR_FAILURE) {
                return ERR_FAILURE;
        } else {
                return ERR_UNFINISHED;
        }
}

enum ErrState run(struct List * itype_list)
{
        LOG_DEBUG("Running a program ...\n");

        if (DEBUG_ON) {
                LOG_DEBUG("Press enter to execute a single step.\n");
        }

        instr_id_t data_stack = find_instr_id(itype_list, g_data_stack_str);
        instr_id_t instr_stack = find_instr_id(itype_list, g_instr_stack_str);

        enum ErrState err_state;
        do {
                if (DEBUG_ON) {
                        (void) getchar();
                        LOG_DEBUG("Stacks:\n");
                        log_itype_list(LOG_LVL_DEBUG, itype_list);
                        LOG_DEBUG("\n\n");
                }

                err_state = step(itype_list, data_stack, instr_stack);

        } while (err_state == ERR_UNFINISHED);

        if (DEBUG_ON) {
                (void) getchar();
        }

        return err_state;
}
