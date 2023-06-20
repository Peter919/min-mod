#include "settings.h"
#include <ctype.h>
#include <string.h>
#include "data_types/itype.h"

const char * const g_minmod_file_ext = "[m]m";

const char * const g_builtin_names[BUILTINS_COUNT] = {"SET", "UNWRAP", "IF"};

const char g_stack_open_ch = '[';
const char g_stack_close_ch = ']';
const char g_indirection_ch = '.';
const char * const g_comment_str = "#";

const char * g_instr_stack_str = "IS";
const char * g_data_stack_str = "DS";

const char g_import_open_ch = '"';
const char g_import_close_ch = '"';

bool is_itype_loggable(const char * itype_name)
{
        return itype_name[0] == '@';
}

bool is_valid_instr_ch(char ch)
{
        if (!isprint(ch)) {
                return false;
        }
        if (isspace(ch)) {
                return false;
        }
        if (ch == g_stack_open_ch || ch == g_stack_close_ch) {
                return false;
        }
        if (ch == g_indirection_ch) {
                return false;
        }
        if (strchr(g_comment_str, ch)) {
                return false;
        }
        if (ch == g_import_open_ch) {
                return false;
        }
        if (ch == g_import_close_ch) {
                return false;
        }
        return true;
}
