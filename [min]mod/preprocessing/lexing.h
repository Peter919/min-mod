#ifndef LEXING_H
#define LEXING_H

// Lexically analyzes the contents of "file_path", returning a list of
// "struct Token"s.
struct List lex(const char * file_path);

#endif
