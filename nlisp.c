#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nlisp.h"

variant var_string_from_range(const char* start, const char* end) {
	return (variant){VAR_STR, .str=strndup(start, end-start)};
}

variant var_string(const char* s) {
	return (variant){VAR_STR, .str=strdup(s)};
}

pair* cons(variant head, variant tail) {
	pair* p = malloc(sizeof(pair));
	*p = (pair){head, tail};
	return p;
}

variant varclone(variant v) {
	if ( ISPAIR(v) ) return VARCONS(varclone(CAR(v)), varclone(CDR(v)));
	if ( ISSTR(v) ) return (variant){VAR_STR, .str=strdup(v.str)};
	return NIL;
}

static void _varprint(variant x, int indent);

static void _varprintlist(variant lst, int indent) {
	if(indent >-1 && indent < 4) { putchar('\n'); for(int i = 0; i <indent; i++) putchar(' '); }
	printf("(");
	for (int first = 1; lst.type != VAR_NIL ; lst=CDR(lst), first=0) {
		if (!first) printf(" ");
		_varprint(CAR(lst), indent >-1?indent+1:-1);
	}
	printf(")");
}

void varprintlist(variant lst) {
	_varprintlist(lst, 0);
}

static void _varprint(variant x, int indent) {
	switch(x.type) {
	case VAR_NIL: printf("NIL"); break;
	case VAR_STR: printf("%s", x.str); break;
	case VAR_PAIR: _varprintlist(x, indent); break;
	default: printf("Unknown type %d\n", x.type); break;
	}


}

void varprint(variant x) {
	_varprint(x, 0);
	putchar('\n');
}

void varinline(variant x) {
	_varprint(x, -1);
}

void varfree(variant x) {
	switch(x.type) {
	case VAR_STR: free(x.str); break;
	case VAR_PAIR: varfree(CAR(x)); varfree(CDR(x)); free(x.pair); break;
	}
}

int vareq(variant a, variant b) {
	if (a.type != b.type ) return 0;
	if (ISNIL(a) && ISNIL(b) ) return 1;
	if (ISSTR(a) && !strcmp(a.str, b.str))  return 1;
	if (ISPAIR(a)) return vareq(CAR(a), CAR(b)) && vareq(CDR(a), CDR(b));
	return 0;
}
