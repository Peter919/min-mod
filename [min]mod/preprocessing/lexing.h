#ifndef LEXING_H
#define LEXING_H

// Returns "true" iff "var_name" belongs to a variable belonging to the
// global scope.
bool is_global(const char * instr_name);

// Assuming "global_instr" is a global variable and is marked as such,
// this returns, without allocating any new data, the unmarked version.
const char * get_unmarked_instr_name(const char * global_instr);

// Lexically analyzes the contents of "file_path", returning a list of
// "struct Token"s.
struct List lex(const char * file_path);

#endif
