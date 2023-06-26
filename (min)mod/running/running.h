#ifndef RUNNING_H
#define RUNNING_H

#include "../tools/list.h"

enum ErrState {
        ERR_FAILURE,
        ERR_UNFINISHED,
        ERR_SUCCESS
};

enum ErrState run(struct List * itype_list, bool debug);

#endif
