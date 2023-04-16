#include "tools/log.h"
#include "tools/list.h"
#include "preprocessing/lexing.h"
#include "preprocessing/parsing.h"
#include "running/running.h"
#include "data_types/itype.h"

int main(int argc, char ** argv)
{
        if (argc != 2) {
                LOG_FATAL_ERROR("Expected 2 arguments "
                       "(file path to [min]mod.exe, file path to .[m]m program), "
                       "got %d.\n", argc);
                proper_exit(EXIT_FAILURE);
        }

        const char * file_path = argv[1];

        struct List token_list = lex(file_path);
        if (!list_is_valid(&token_list)) {
                LOG_FATAL_ERROR("Failed to lex \"%s\".\n", file_path);
                proper_exit(EXIT_FAILURE);
        }

        struct List itype_list = parse(&token_list);

        enum ErrState ret_val = run(&itype_list);

        LOG(LOG_LVL_CONSOLE, "Final stacks:\n");
        log_itype_list(LOG_LVL_CONSOLE, &itype_list);
        LOG(LOG_LVL_CONSOLE, "\n");

        if (ret_val == ERR_FAILURE) {
                proper_exit(EXIT_FAILURE);
        } else {
                proper_exit(EXIT_SUCCESS);
        }
}
