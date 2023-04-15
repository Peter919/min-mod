#include "parsing.h"
#include "../data_types/itype.h"
#include "token.h"
#include "../tools/log.h"
#include "../data_types/stack.h"
#include "../settings.h"
#include "../tools/mem_tools.h"

static struct List tokens_to_itype_list(const struct List * tokens)
{
        struct List itype_list = create_list(sizeof(struct IType), destroy_itype_void_ptr);

        // IS and DS must be in the list even if they're not referenced in the
        // program, as they're implicitly used by the built-in instructions.
        add_itype_to_list(&itype_list, g_instr_stack_str);
        add_itype_to_list(&itype_list, g_data_stack_str);

        for (size_t i = 0; i < tokens->length; ++i) {

                const struct Token * curr_tok = get_list_elem_const(tokens, i);

                if (curr_tok->type == TOK_INSTR &&
                    !itype_in_list(&itype_list, curr_tok->instr_name)) {
                        add_itype_to_list(&itype_list, curr_tok->instr_name);
                }
        }

        return itype_list;
}

static struct Stack * tokens_to_stack(const struct List * tokens, const struct List * itype_list);

static struct StackElem get_nested_stack(const struct List * tokens,
                                         const struct List * itype_list,
                                         size_t * iterator)
{
        size_t stack_open_idx = *iterator;
        const struct Token * stack_open_tok = get_list_elem_const(tokens, stack_open_idx);

        int nesting_level = 0;
        do {
                ASSERT_OR_HANDLE(*iterator < tokens->length, create_invalid_stack_elem(),
                                 "Unclosed %s in line %d.",
                                 token_type_as_string(stack_open_tok->type), stack_open_tok->line);

                const struct Token * curr_tok = get_list_elem_const(tokens, *iterator);
                switch (curr_tok->type) {
                case TOK_STACK_OPEN:
                        ++nesting_level;
                        break;
                case TOK_STACK_CLOSE:
                        --nesting_level;
                        break;
                default:
                        break;
                }

                ++*iterator;

        } while (nesting_level > 0);

        size_t stack_close_idx = *iterator - 1;
        struct List sublist = create_sublist(tokens, stack_open_idx + 1, stack_close_idx);

        int indirection_level = 0;
        if (*iterator < tokens->length) {

                const struct Token * tok_after_substack = get_list_elem_const(tokens, *iterator);

                if (tok_after_substack->type == TOK_INDIRECTION) {
                        indirection_level = tok_after_substack->indirection_level;
                        ++*iterator;
                }
        }

        struct Stack * stack = tokens_to_stack(&sublist, itype_list);
        struct StackElem stack_as_substack = stack_to_stack_elem(stack, indirection_level);

        return stack_as_substack;
}

static struct StackElem get_instr(const struct List * tokens,
                                  const struct List * itype_list,
                                  size_t * iterator)
{
        const struct Token * instr_tok = get_list_elem_const(tokens, *iterator);
        ++*iterator;

        int indirection_level = 0;

        if (*iterator < tokens->length) {
                const struct Token * next_tok = get_list_elem_const(tokens, *iterator);
                if (next_tok->type == TOK_INDIRECTION) {
                        indirection_level = next_tok->indirection_level;
                        ++*iterator;
                }
        }

        instr_id_t instr = find_instr_id(itype_list, instr_tok->instr_name);

        struct StackElem instr_as_stack_elem = instr_to_stack_elem(instr, indirection_level);
        return instr_as_stack_elem;
}

static struct Stack * tokens_to_stack(const struct List * tokens, const struct List * itype_list)
{
        struct Stack * stack = create_stack();

        size_t idx = 0;
        while (idx < tokens->length) {

                const struct Token * curr_tok = get_list_elem_const(tokens, idx);

                switch (curr_tok->type) {
                case TOK_STACK_OPEN: {
                        struct StackElem stack_elem = get_nested_stack(tokens, itype_list, &idx);
                        stack_push(stack, &stack_elem);
                        break;

                } case TOK_INSTR: {
                        struct StackElem stack_elem = get_instr(tokens, itype_list, &idx);
                        stack_push(stack, &stack_elem);
                        break;
                } default: {
                        ASSERT_OR_HANDLE(false, create_invalid_stack(), "Unexpected %s in line %d.",
                                         token_type_as_string(curr_tok->type), curr_tok->line);

                }
                }
        }

        reverse_stack(stack);
        return stack;
}

static void set_builtin_value(enum Builtin builtin, struct List * itype_list)
{
        const char * builtin_name = g_builtin_names[builtin];

        if (itype_in_list(itype_list, builtin_name)) {
                struct IType * itype = instr_name_to_itype(itype_list, builtin_name);
                itype->value = create_stack();

                // Built-ins have indices -1, -2, -3 etc.
                instr_id_t instr = builtin_to_id(builtin);
                struct StackElem stack_elem = instr_to_stack_elem(instr, 0);

                stack_push(itype->value, &stack_elem);
        }
}

struct List parse(const struct List * tokens)
{
        struct List itype_list = tokens_to_itype_list(tokens);

        LOG_DEBUG("Instruction types:\n");
        log_itype_list(LOG_LVL_DEBUG, &itype_list);
        LOG_DEBUG("\n\n");

        for (enum Builtin i = 0; i < BUILTINS_COUNT; ++i) {
                set_builtin_value(i, &itype_list);
        }

        struct IType * data_stack = instr_name_to_itype(&itype_list, g_data_stack_str);
        data_stack->value = create_stack();

        struct Stack * instr_substack = tokens_to_stack(tokens, &itype_list);
        struct StackElem instr_substack_as_stack_elem = stack_to_stack_elem(instr_substack, 0);

        struct IType * instr_stack = instr_name_to_itype(&itype_list, g_instr_stack_str);
        instr_stack->value = create_stack();
        stack_push(instr_stack->value, &instr_substack_as_stack_elem);

        LOG_DEBUG("%s (backwards for better readability):\n", g_instr_stack_str);
        log_stack_backwards(LOG_LVL_DEBUG, instr_stack->value, &itype_list);
        LOG_DEBUG("\n\n");

        return itype_list;
}
