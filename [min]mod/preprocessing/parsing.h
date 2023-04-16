#ifndef PARSING_H
#define PARSING_H

#include "../tools/list.h"

// Converts a list of tokens to a list of stacks, including the instruction
// stack and data stack (which both have arbitrary indices within the list
// but correct names).
struct List parse(const struct List * tokens);

#endif
