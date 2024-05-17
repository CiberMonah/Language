#ifndef _BUILDER_H_
#define _BUILDER_H_

#include "nlisp.h"
#include "vartable.h"

variant build_expr(variant conseq,  VarTable* t, variant rules);

#endif /* _BUILDER_H_ */
