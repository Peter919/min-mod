#include <ctype.h>
#include <string.h>
#include "../tools/debug.h"
#include "../settings.h"
#include "../tools/file_operations.h"
#include "../tools/list.h"
#include "token.h"
#include "../tools/mem_tools.h"

// Not beautiful, but it gets the job done :D
// Makes "*iterator" skip past the current token, incrementing "*line"
// if newlines are found.
// Returns the current token (the one "*iterator" skips past).
static struct Token scan_token(const char ** iterator, int * line)
{
        // Match **iterator against a bunch of different tokens ...
        if (**iterator == g_stack_open_ch) {
                ++*iterator;
                return create_token(TOK_STACK_OPEN, *line);

        } else if (**iterator == g_stack_close_ch) {
                ++*iterator;
                return create_token(TOK_STACK_CLOSE, *line);

        } else if (**iterator == g_indirection_ch) {
                int indirection_level = 0;
                while (**iterator == g_indirection_ch) {
                        ++*iterator;
                        ++indirection_level;
                }

                struct Token token = create_token(TOK_INDIRECTION, *line);
                token.indirection_level = indirection_level;

                return token;

        } else if (isspace(**iterator)) {
                while (isspace(**iterator)) {
                        if (**iterator == '\n') {
                                ++*line;
                        }
                        ++*iterator;
                }
                return create_token(TOK_WHITESPACE, *line);

        } else if (strncmp(*iterator, g_comment_str, strlen(g_comment_str)) == 0) {
                while (**iterator && **iterator != '\n') {
                        ++*iterator;
                }
                return create_token(TOK_COMMENT, *line);
        }

        // If this point is reached, "iterator" has been matched against all
        // the fun tokens, and the only possibility left is plain, boring
        // "TOK_INSTR".

        ASSERT_OR_HANDLE(is_valid_instr_ch(**iterator), create_token(TOK_INVALID, -1),
                         "Invalid character \"%c\" in line %d.", **iterator, *line);

        const char * instr_start = *iterator;

        while (is_valid_instr_ch(**iterator)) {
                ++*iterator;
        }
        int instr_length = *iterator - instr_start;

        char * instr_name = ALLOC(char, instr_length + 1);
        strncpy(instr_name, instr_start, instr_length);
        instr_name[instr_length] = '\0';

        struct Token token = create_token(TOK_INSTR, *line);
        token.instr_name = instr_name;

        return token;
}

// Remove all tokens in "token_list" of type "type".
static void remove_tokens_of_type(struct List * token_list, enum TokenType type)
{
        LOG_INFO("Removing all %s tokens from " LIST_FS " ...\n",
                 token_type_as_string(type),
                 LIST_FA(*token_list));

        for (size_t i = 0; i < token_list->length; ++i) {

                const struct Token * curr_tok = get_list_elem_const(token_list, i);
                if (curr_tok->type == type) {
                        list_remove(token_list, i);
                        --i;
                }
        }
}

// Lexes "string".
static struct List string_to_tokens(const char * string)
{
        LOG_INFO("Converting string to list of tokens ...\n");

        struct List token_list = create_list(sizeof(struct Token), destroy_token_void_ptr);
        const char * iterator = string;
        int line = 1;

        while (*iterator) {
                struct Token curr_tok = scan_token(&iterator, &line);
                if (curr_tok.type == TOK_INVALID) {
                        return create_invalid_list();
                }
                list_append(&token_list, &curr_tok);
        }

        return token_list;
};

struct List lex(const char * file_path)
{
        LOG_INFO("Lexing \"%s\" ...\n", file_path);

        const char * file_ext = get_file_ext(file_path);

        ASSERT_OR_HANDLE(file_ext, create_invalid_list(),
                         "Cannot lex file \"%s\", as it has no extension.", file_path);

        ASSERT_OR_HANDLE(strcmp(file_ext, g_minmod_file_ext) == 0, create_invalid_list(),
                         "[min]mod files must have extension \"%s\", "
                         "but \"%s\" has extension \"%s\".",
                         g_minmod_file_ext, file_path, file_ext);

        const char * file_contents = file_to_string(file_path);

        LOG_DEBUG("\"%s\":\n\"%s\"\n\n", file_path, file_contents);

        struct List token_list = string_to_tokens(file_contents);
        if (!list_is_valid(&token_list)) {
                return create_invalid_list();
        }

        remove_tokens_of_type(&token_list, TOK_WHITESPACE);
        remove_tokens_of_type(&token_list, TOK_COMMENT);

        LOG_DEBUG("Tokens in \"%s\":\n", file_path);
        log_token_list(LOG_LVL_DEBUG, &token_list);
        LOG_DEBUG("\n\n");

        return token_list;
}
