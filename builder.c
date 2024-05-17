#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "vartable.h"
#include "nlisp.h"
#include "builder.h"
#include "pattern.h"

variant _build_list(variant bpat, VarTable* t, variant rules) {
	if ( ISNIL(bpat) ) return NIL;
	else if ( ISPAIR(bpat) ) {
		variant head = CAR(bpat);
		/* special case for ellipsis */
		if ( ISPAIR(head) && ISSTR(CAR(head)) && VARSTREQ(CAR(head), "ELLIPSIS")) {
			variant res;
			if ( vartable_get(t, CADR(head).str, &res) ) return varclone(res);
			return (printf("NO Ellipsis '%s' unified\n", CADR(head).str), NIL); 
		} 
		return VARCONS(build_expr(CAR(bpat), t, rules), _build_list(CDR(bpat), t, rules));
	}
	return NIL;
}

variant apply_builder(variant bpat, VarTable* t, variant rules) {
	variant res = VARCONS(var_string("APPLY"), _build_list(bpat, t, rules));
	apply_rules(&res, rules);
	return res;
}

variant list_builder(variant bpat, VarTable* t, variant rules) {
	return _build_list(bpat, t, rules);	
}

#define ISNUM(x) ISPAIR(x) && ISSTR(CAR(x)) && VARSTREQ(CAR(x), "NUM")

variant eval_builder(variant bpat, VarTable* t, variant rules) {
	// do simple evaluation
	char buf[32];
	double result, arg1, arg2;
	int done = 0;
	variant lst = _build_list(bpat, t, rules); // recursively build list
	if ( ISPAIR(lst) && ISSTR(CAR(lst)) ) { 
		variant head = CAR(lst), tail = CDR(lst);
		if ( ISPAIR(tail) && ISPAIR(CDR(tail)) ) {
			if ( ISNUM(CAR(tail))  && ISNUM(CADR(tail))) {
				arg1 = atof(CADR(CAR(tail)).str), arg2 = atof(CADR(CADR(tail)).str);
				if (VARSTREQ(head, "+")) (done=1, result = arg1+arg2);
				else if (VARSTREQ(head, "-")) (done=1, result = arg1-arg2);
				else if (VARSTREQ(head, "*")) (done=1, result = arg1*arg2);
				else if (VARSTREQ(head, "/")) (done=1, result = arg1/arg2);
				else if (VARSTREQ(head, "^")) (done=1, result = pow(arg1, arg2));
			}
		} else if ( ISPAIR(tail) &&  ISNUM(CAR(tail))) {
			arg1 = atof(CADR(CAR(tail)).str);
			if (VARSTREQ(head, "-")) (done=1, result = -arg1);
		}
	} 
	if (done) {
		if (rint(result) == result)
			sprintf(buf, "%.0lf", result);
		else
			sprintf(buf, "%lf", result);
		varfree(lst); // drop list
		return VARCONS(var_string("NUM"), VARCONS(var_string(buf), NIL));
	}
	return lst; // in all other cases return list as is
}

variant scalar_builder(variant bpat, VarTable* t, variant rules) {
	variant res;
	if ( vartable_get(t, CAR(bpat).str,  &res) ) return varclone(res);
	return (printf("NO var '%s' unified\n", CAR(bpat).str), NIL);
}

variant asis_builder(variant bpat, VarTable* t, variant rules) {
	return varclone(CAR(bpat));
}

variant num_builder(variant bpat, VarTable* t, variant rules) {
	variant res;
	if ( vartable_get(t, CAR(bpat).str, &res)) return VARCONS(var_string("NUM"), VARCONS(varclone(res), NIL));
	return (printf("NO const '%s' unified\n", CAR(bpat).str), NIL);
}

variant var_builder(variant bpat, VarTable* t, variant rules) {
	variant res;
	if ( vartable_get(t, CAR(bpat).str, &res)) return VARCONS(var_string("VAR"), VARCONS(varclone(res), NIL));
	return (printf("NO var '%s' unified\n", CAR(bpat).str), NIL);
}

struct builder {
	const char* name;
	variant (*buildfunc)(variant, VarTable*, variant);
};
struct builder builders[] = {
	{"APPLY", apply_builder}, {"LIST", list_builder},
	{"CONSTM", num_builder}, {"VARM", var_builder},
	{"ANYM", scalar_builder}, {"ELLIPSIS", scalar_builder},
	{"NAME", asis_builder}, {"NUM", asis_builder}, {"EVAL", eval_builder},
	{NULL, NULL}};

variant build_expr(variant conseq, VarTable* t, variant rules) {
	variant head = CAR(conseq);
	for (struct builder* b = builders; b->name; b++ ) 
		if (VARSTREQ(head, b->name)) {
			//printf("Building with ");
			//varinline(CDR(conseq));
			//printf("\n");
			return b->buildfunc(CDR(conseq), t, rules);
		}
	/* otherwise, copy as-is */
	return varclone(conseq);
}
