#include "itype.h"
#include <string.h>
#include "../tools/list.h"
#include "../settings.h"
#include "stack.h"
#include "../tools/mem_tools.h"

void destroy_itype(struct IType * itype)
{
        ASSERT(false, "Cannot destroy instruction types.");
}

void destroy_itype_void_ptr(void * itype)
{
        destroy_itype(itype);
}

bool itype_in_list(const struct List * itype_list, const char * instr_name)
{
        for (size_t i = 0; i < itype_list->length; ++i) {

                const struct IType * curr_itype = get_list_elem_const(itype_list, i);

                if (strcmp(curr_itype->name, instr_name) == 0) {
                        return true;
                }
        }

        return false;
}

static struct IType create_itype(const char * name)
{
        struct IType itype;
        itype.value = NULL;

        itype.name = ALLOC(char, strlen(name) + 1);
        strcpy(itype.name, name);

        return itype;
}

void add_itype_to_list(struct List * itype_list, const char * instr_name)
{
        ASSERT(!itype_in_list(itype_list, instr_name),
               "Instruction %s already in instruction " LIST_FS ".",
               instr_name, LIST_FA(*itype_list));

        struct IType new_itype = create_itype(instr_name);
        list_append(itype_list, &new_itype);
}

instr_id_t find_instr_id(const struct List * itype_list, const char * instr_name)
{
        for (size_t i = 0; i < itype_list->length; ++i) {

                const struct IType * curr_itype = get_list_elem_const(itype_list, i);

                if (strcmp(curr_itype->name, instr_name) == 0) {
                        return (instr_id_t) i;
                }
        }

        ASSERT(false, "Could not find instruction %s in instruction " LIST_FS ".",
               instr_name, LIST_FA(*itype_list));

        return -1;
}

struct IType * id_to_itype(struct List * itype_list, instr_id_t id)
{
        return get_list_elem(itype_list, id);
}

const struct IType * id_to_itype_const(const struct List * itype_list, instr_id_t id)
{
        return get_list_elem_const(itype_list, id);
}

struct IType * instr_name_to_itype(struct List * itype_list, const char * instr_name)
{
        instr_id_t id = find_instr_id(itype_list, instr_name);
        return id_to_itype(itype_list, id);
}

static bool must_be_logged(const char * itype_name)
{
        if (strcmp(itype_name, g_instr_stack_str) == 0) {
                return true;
        }
        if (strcmp(itype_name, g_data_stack_str) == 0) {
                return true;
        }
        return false;
}

void log_itype_list(int log_level, const struct List * list)
{
        if (!LOGGABLE(log_level)) {
                return;
        }

        bool first_logging = true;
        for (size_t i = 0; i < list->length; ++i) {

                const struct IType * curr_itype = get_list_elem_const(list, i);

                if (!(is_itype_loggable(curr_itype->name) || must_be_logged(curr_itype->name))) {
                        continue;
                }

                if (first_logging) {
                        first_logging = false;
                } else {
                        LOG(log_level, "\n");
                }

                LOG(log_level, "%s: ", curr_itype->name);

                if (curr_itype->value) {
                        log_stack_backwards(log_level, curr_itype->value, list);
                } else {
                        LOG(log_level, "(uninitialized)");
                }
        }
}

bool is_builtin(instr_id_t id)
{
        return id < 0;
}

instr_id_t builtin_to_id(enum Builtin builtin)
{
        return -builtin - 1;
}

enum Builtin id_to_builtin(instr_id_t id)
{
        return -id - 1;
}
