#ifndef INSTR_TYPE_H
#define INSTR_TYPE_H

#include <stdbool.h>
#include "../tools/list.h"

typedef int instr_id_t;

enum Builtin {
        BUILTIN_SET,
        BUILTIN_UNWRAP,
        BUILTIN_IF,
        BUILTINS_COUNT
};

struct IType {
        char * name;
        struct Stack * value;
};

// No "create_itype" function since they're only supposed to be created
// by adding them to a list using "add_itype_to_list".

void destroy_itype(struct IType * itype);

void destroy_itype_void_ptr(void * itype);

// Returns "true" if and only if "itype_list" contains a "struct IType" named
// "instr_name".
bool itype_in_list(const struct List * itype_list, const char * instr_name);

// Adds a new "struct IType" to "itype_list", named "instr_name".
// "itype_list" cannot already contain an instruction type with that name.
void add_itype_to_list(struct List * itype_list, const char * instr_name);

// Return the ID of the "struct IType" in "itype_list" named "instr_name".
// Can only be called if such "struct IType" exists.
instr_id_t find_instr_id(const struct List * itype_list, const char * instr_name);

// Returns the name of the instruction type with ID "id" in "itype_list".
// Can only be called if the ID is valid within the list.
struct IType * id_to_itype(struct List * itype_list, instr_id_t id);

// Same as "id_to_itype", but the argument can be constant at the cost of
// a constant return value.
const struct IType * id_to_itype_const(const struct List * itype_list, instr_id_t id);

// Returns the instruction type named "instr_name" in "itype_list".
// Only legal if such instruction type exists.
struct IType * instr_name_to_itype(struct List * itype_list, const char * instr_name);

void log_itype_list(int log_level, const struct List * list);

bool is_builtin(instr_id_t id);

instr_id_t builtin_to_id(enum Builtin builtin);

enum Builtin id_to_builtin(instr_id_t id);

#endif
