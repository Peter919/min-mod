#include <ctype.h>
#include <string.h>
#include "../tools/debug.h"
#include "../settings.h"
#include "../tools/file_operations.h"
#include "../tools/list.h"
#include "token.h"
#include "../tools/mem_tools.h"
#include "../data_types/itype.h"

static char * str_between_ptrs(const char * start, const char * end)
{
        // + 1 since the range is inclusive.
        int len = end - start + 1;

        char * new_str = ALLOC(char, len + 1);
        strncpy(new_str, start, len);
        new_str[len] = '\0';

        return new_str;
}

// Not beautiful, but it gets the job done :D
// Makes "*iterator" skip past the current token, incrementing "*line"
// if newlines are found.
// Returns the current token (the one "*iterator" skips past).
static struct Token scan_token(const char ** iterator, const char * file, int * line)
{
        // Match **iterator against a bunch of different tokens ...
        if (**iterator == g_import_open_ch) {

                const char * import_open = *iterator;

                do {
                        ++*iterator;

                        ASSERT_OR_HANDLE(**iterator && **iterator != '\n', create_invalid_token(),
                                         "Unclosed \"%c\" in line %d.",
                                         *import_open, *line);
                } while (**iterator != g_import_close_ch);
                ++*iterator;

                const char * import_close = *iterator - 1;

                char * fname = str_between_ptrs(import_open + 1, import_close - 1);

                struct Token token = create_token(TOK_IMPORT, file, *line);
                token.import_file = fname;

                return token;

        } else if (**iterator == g_stack_open_ch) {
                ++*iterator;
                return create_token(TOK_STACK_OPEN, file, *line);

        } else if (**iterator == g_stack_close_ch) {
                ++*iterator;
                return create_token(TOK_STACK_CLOSE, file, *line);

        } else if (**iterator == g_indirection_ch) {
                int indirection_level = 0;
                while (**iterator == g_indirection_ch) {
                        ++*iterator;
                        ++indirection_level;
                }

                struct Token token = create_token(TOK_INDIRECTION, file, *line);
                token.indirection_level = indirection_level;

                return token;

        } else if (isspace(**iterator)) {
                while (isspace(**iterator)) {
                        if (**iterator == '\n') {
                                ++*line;
                        }
                        ++*iterator;
                }
                return create_token(TOK_WHITESPACE, file, *line);

        } else if (strncmp(*iterator, g_comment_str, strlen(g_comment_str)) == 0) {
                while (**iterator && **iterator != '\n') {
                        ++*iterator;
                }
                return create_token(TOK_COMMENT, file, *line);
        }

        // If this point is reached, "iterator" has been matched against all
        // the fun tokens, and the only possibility left is plain, boring
        // "TOK_INSTR".

        ASSERT_OR_HANDLE(is_valid_instr_ch(**iterator), create_invalid_token(),
                         "Invalid character \"%c\" in line %d.", **iterator, *line);

        const char * instr_start = *iterator;

        while (is_valid_instr_ch(**iterator)) {
                ++*iterator;
        }
        int instr_length = *iterator - instr_start;

        char * instr_name = ALLOC(char, instr_length + 1);
        strncpy(instr_name, instr_start, instr_length);
        instr_name[instr_length] = '\0';

        struct Token token = create_token(TOK_INSTR, file, *line);
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
static struct List string_to_tokens(const char * string, const char * fname)
{
        LOG_INFO("Converting string to list of tokens ...\n");

        struct List token_list = create_list(sizeof(struct Token), destroy_token_void_ptr);
        const char * iterator = string;
        int line = 1;

        while (*iterator) {
                struct Token curr_tok = scan_token(&iterator, fname, &line);
                if (curr_tok.type == TOK_INVALID) {
                        return create_invalid_list();
                }
                list_append(&token_list, &curr_tok);
        }

        return token_list;
};

// Returns "true" iff the "TOK_STACK_OPEN"s and "TOK_STACK_CLOSE"s are balanced.
// Assumes all tokens in "tokens" belong to the same file.
static bool are_parens_balanced(const struct List * tokens)
{
        int nesting_level = 0;
        for (int i = 0; i < tokens->length; ++i) {
                const struct Token * curr_tok = get_list_elem_const(tokens, i);

                if (curr_tok->type == TOK_STACK_OPEN) {
                        ++nesting_level;
                } else if (curr_tok->type == TOK_STACK_CLOSE) {
                        --nesting_level;
                }

                ASSERT_OR_HANDLE(nesting_level >= 0,
                                "Unexpected %s in \"%s\", line %d.", false,
                                token_type_as_string(curr_tok->type),
                                curr_tok->file, curr_tok->line);
        }
        ASSERT_OR_HANDLE(nesting_level == 0, false,
                         "Unclosed %s.", token_type_as_string(TOK_STACK_OPEN));

        return true;
}

static bool str_in_list(const struct List * str_list, const char * str)
{
        for (int i = 0; i < str_list->length; ++i) {
                const char * curr_elem = get_list_elem_const(str_list, i);
                if (strcmp(str, curr_elem) == 0) {
                        return true;
                }
        }
        return false;
}

static struct List tokenize_file(char * file_path, struct List * tokenized_files);

// Like "strdup", but it uses the "ALLOC" macro rather than "malloc".
static char * duplicate_str(const char * str)
{
        char * new_str = ALLOC(char, strlen(str) + 1);
        strcpy(new_str, str);
        return new_str;
}

static char * get_nth_parent_dir(const char * fpath, int parent_count)
{
        char * curr_parent = duplicate_str(fpath);
        for (int i = 0; i < parent_count; ++i) {
                char * next_parent = get_parent_dir(curr_parent);

                ASSERT_OR_HANDLE(next_parent, NULL,
                                 "\"%s\" has no parent directory.", curr_parent);

                FREE(curr_parent);
                curr_parent = next_parent;
        }

        return curr_parent;
}

static char * get_abs_import_path(const char * fpath, const char * import)
{
        if (import[0] != '.') {
                return duplicate_str(import);
        }

        int parent_count = 0;
        for (int i = 0; import[i] == '.'; ++i) {
                ++parent_count;
        }

        const char * child_path = import + parent_count;
        char * parent_path = get_nth_parent_dir(fpath, parent_count);

        char * abs_path = ALLOC(char, strlen(parent_path) + strlen(child_path) + 1);
        abs_path[0] = '\0';

        strcat(abs_path, parent_path);
        strcat(abs_path, child_path);

        FREE(parent_path);

        return abs_path;
}

// Returns "true" iff there's no errors.
static bool evaluate_imports(struct List * tokens, struct List * tokenized_files)
{
        for (int i = 0; i < tokens->length; ++i) {
                const struct Token * curr_tok = get_list_elem_const(tokens, i);
                if (curr_tok->type != TOK_IMPORT) {
                        continue;
                }

                char * abs_import_path = get_abs_import_path(curr_tok->file, curr_tok->import_file);

                if (str_in_list(tokenized_files, abs_import_path)) {
                        continue;
                }

                struct List import_tokens = tokenize_file(abs_import_path, tokenized_files);

                ASSERT_OR_HANDLE(list_is_valid(&import_tokens), false,
                                 "Unable to tokenize \"%s\".", abs_import_path);

                list_remove(tokens, i);
                list_insert_list(tokens, i, &import_tokens);

                // Since the import token is now removed, we must stay at the same
                // index one more iteration to start form the beginning of the
                // new tokens. Therefore, we decrement "i" before incrementing it
                // when we start the next iteration.
                // One's not really supposed to mess with the loop index, but making
                // this a while loop only seemed to add more clutter since "i"
                // wouldn't be incremented when using "continue".
                --i;
        }

        return true;
}

// Converts "file_path" to a list of tokens. Additionally, it recursively
// tokenizes any file included by "file_path" and pastes them into wherever
// the files were included.
static struct List tokenize_file(char * file_path, struct List * tokenized_files)
{
        LOG_INFO("Tokenizing \"%s\" ...\n", file_path);

        list_append(tokenized_files, &file_path);

        const char * file_ext = get_file_ext(file_path);

        ASSERT_OR_HANDLE(file_ext, create_invalid_list(),
                         "Cannot lex file \"%s\", as it has no extension.", file_path);

        ASSERT_OR_HANDLE(strcmp(file_ext, g_minmod_file_ext) == 0, create_invalid_list(),
                         "(min)mod files must have extension \"%s\", "
                         "but \"%s\" has extension \"%s\".",
                         g_minmod_file_ext, file_path, file_ext);

        const char * file_contents = file_to_string(file_path);

        ASSERT_OR_HANDLE(file_contents, create_invalid_list(),
                         "Unable to extract the contents of \"%s\".", file_path);

        LOG_DEBUG("\"%s\":\n\"%s\"\n\n", file_path, file_contents);

        struct List token_list = string_to_tokens(file_contents, file_path);
        if (!list_is_valid(&token_list)) {
                return create_invalid_list();
        }

        remove_tokens_of_type(&token_list, TOK_WHITESPACE);
        remove_tokens_of_type(&token_list, TOK_COMMENT);

        ASSERT_OR_HANDLE(are_parens_balanced(&token_list), create_invalid_list(),
                         "Unbalanced parentheses in \"%s\".", file_path);

        bool imports_ok = evaluate_imports(&token_list, tokenized_files);

        ASSERT_OR_HANDLE(imports_ok, create_invalid_list(),
                         "Unable to evaluate the imports of \"%s\".", file_path);

        LOG_DEBUG("Tokens in \"%s\":\n", file_path);
        log_token_list(LOG_LVL_DEBUG, &token_list);
        LOG_DEBUG("\n\n");

        return token_list;
}

static void free_str(void * str)
{
        FREE(*(char **) str);
}

struct List lex(const char * file_path)
{
        LOG_INFO("Lexing \"%s\" ...\n", file_path);

        struct List tokenized_files = create_list(sizeof(char *), free_str);

        // "tokenize_file" expects an "ALLOC"ed file path that will be freed
        // when "destroy_list" is called, and we want "lex" to be polite enough
        // to not "FREE" its argument. Therefore, we copy the argument.
        char * file_path_cpy = duplicate_str(file_path);
        struct List token_list = tokenize_file(file_path_cpy, &tokenized_files);

        destroy_list(&tokenized_files);

        return token_list;
}
