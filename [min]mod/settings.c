#include "settings.h"
#include <ctype.h>
#include "data_types/itype.h"

const char * const g_minmod_file_ext = "[m]m";

const char * const g_builtin_names[BUILTINS_COUNT] = {"SET", "MOVE", "IF"};

const char g_stack_open_ch = '[';
const char g_stack_close_ch = ']';
const char g_indirection_ch = '.';
const char * const g_comment_str = "--";

const char * g_instr_stack_str = "IS";
const char * g_data_stack_str = "DS";

bool is_valid_instr_ch(char ch)
{
        return isalnum(ch) || ch == '_';
}
