#include "token.h"
#include <stdlib.h>
#include "../tools/log.h"
#include "../tools/mem_tools.h"

#define INVALID_INDIRECTION 0

struct Token create_token(enum TokenType type, int line)
{
        struct Token tok;
        tok.type = type;
        tok.line = line;

        // Initialize data to invalid values.
        switch (tok.type) {
        case TOK_INSTR:
                tok.instr_name = NULL;
                break;
        case TOK_INDIRECTION:
                tok.indirection_level = INVALID_INDIRECTION;
                break;
        default:
                break;
        }

        return tok;
}

void destroy_token(struct Token * token)
{
        if (token->type == TOK_INSTR && token->instr_name) {
                FREE(token->instr_name);
        }
}

void destroy_token_void_ptr(void * token)
{
        destroy_token(token);
}

const char * token_type_as_string(enum TokenType token_type)
{
        switch (token_type) {
        case TOK_WHITESPACE:
                return "WHITESPACE";
        case TOK_COMMENT:
                return "COMMENT";
        case TOK_INSTR:
                return "INSTRUCTION";
        case TOK_INDIRECTION:
                return "INDIRECTION";
        case TOK_STACK_OPEN:
                return "STACK OPEN";
        case TOK_STACK_CLOSE:
                return "STACK CLOSE";
        // This is not a default case because we want warnings if any enumerated
        // constants are missing.
        case TOK_INVALID:
                break;
        }

        return "(invalid token)";
}

void log_token(int log_level, const struct Token * tok)
{
        if (!LOGGABLE(log_level)) {
                return;
        }

        LOG(log_level, "%s", token_type_as_string(tok->type));

        switch (tok->type) {
        case TOK_INSTR:
                if (tok->instr_name) {
                        LOG(log_level, " (%s)", tok->instr_name);
                }
                break;
        case TOK_INDIRECTION:
                if (tok->indirection_level != INVALID_INDIRECTION) {
                        LOG(log_level, " (%d)", tok->indirection_level);
                }
                break;
        default:
                break;
        }
}

static void log_newlines(int log_level, int start, int end)
{
        for (int i = start; i <= end; ++i) {
                LOG(log_level, "\n%d\t", i);
        }
}

void log_token_list(int log_level, const struct List * list)
{
        if (!LOGGABLE(log_level)) {
                return;
        }

        // Start at 0 so the first line will be logged.
        int line = 0;
        for (size_t i = 0; i < list->length; ++i) {

                const struct Token * curr_tok = get_list_elem_const(list, i);

                if (curr_tok->line > line) {
                        log_newlines(log_level, line + 1, curr_tok->line);
                        line = curr_tok->line;
                } else {
                        LOG(log_level, ", ");
                }

                log_token(log_level, curr_tok);
        }
}
