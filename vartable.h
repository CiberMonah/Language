#ifndef _VARTABLE_H_
#define _VARTABLE_H_

#include "nlisp.h"

typedef struct _vartable {
	char* name;
	variant value;
	struct _vartable* next;
} VarTable;

void vartable_set(VarTable** t, const char* name, variant value);
int vartable_get(VarTable* t, const char* name, variant* value);
void vartable_free(VarTable* t);

#endif /* _VARTABLE_H_ */
