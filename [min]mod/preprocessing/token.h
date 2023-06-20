#ifndef TOKEN_H
#define TOKEN_H

#include "../tools/list.h"

enum TokenType {
        TOK_INVALID,
        TOK_WHITESPACE,
        TOK_COMMENT,
        TOK_IMPORT,
        TOK_INSTR,
        TOK_INDIRECTION,
        TOK_STACK_OPEN,
        TOK_STACK_CLOSE
};

struct Token {
        enum TokenType type;
        union {
                char * instr_name;
                char * import_file;
                int indirection_level;
        };
        const char * file;
        int line;
};

struct Token create_token(enum TokenType type, const char * file, int line);

struct Token create_invalid_token(void);

void destroy_token(struct Token * token);

void destroy_token_void_ptr(void * token);

const char * token_type_as_string(enum TokenType token_type);

void log_token(int log_level, const struct Token * tok);

void log_token_list(int log_level, const struct List * list);

#endif
