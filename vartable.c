#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vartable.h"
#include "nlisp.h"

void vartable_set(VarTable** t, const char* name, variant val) {
	VarTable* n = malloc(sizeof(VarTable));
	//printf("Setting "); varinline(val); printf(" to %s\n", name);
	*n = (VarTable){strdup(name), val, *t};
	//printf("New table %p -> %p\n", *t, n);
	*t = n;
}

int vartable_get(VarTable* t, const char* name, variant* res) {
	if ( !t ) return 0;
	if ( !strcmp(t->name, name) ) {
		//printf("Getting "); varinline(t->value); printf(" from %s table %p\n", name, t);
		*res = t->value;
		return 1;

	}
	return vartable_get(t->next, name, res);
}

void vartable_free(VarTable* t) {
	if (!t ) return;
	VarTable* n = t->next;
	free(t->name);
	free(t);
	vartable_free(n);
}
