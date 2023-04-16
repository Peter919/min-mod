#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

extern const char * const g_minmod_file_ext;

extern const char * const g_builtin_names[];

extern const char g_stack_open_ch;
extern const char g_stack_close_ch;
extern const char g_indirection_ch;
extern const char * const g_comment_str;

extern const char * g_instr_stack_str;
extern const char * g_data_stack_str;

bool is_valid_instr_ch(char ch);

#endif
