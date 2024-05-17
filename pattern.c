#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vartable.h"
#include "nlisp.h"
#include "pattern.h"
#include "builder.h"


int pattern_match(variant expr, variant ant, VarTable** t);

static int list_match(variant expr, variant pattern, VarTable** t) {
	if (ISNIL(expr) && ISNIL(pattern)) return 1;
	if ((ISNIL(expr) || ISPAIR(expr)) && ISPAIR(pattern)) {
		/* check special ellipsis case */
		variant p = CAR(pattern);
		if (ISPAIR(p) && ISSTR(CAR(p)) && VARSTREQ(CAR(p), "ELLIPSIS")){ 
			vartable_set(t, CADR(p).str, expr); 
			return 1;
		} 
		if ( ISPAIR(expr) && pattern_match(CAR(expr), p, t) )
			return list_match(CDR(expr), CDR(pattern), t);
	}
	return 0;
}

static int apply_match(variant expr, variant pattern, VarTable** t) {
	if (ISPAIR(expr) && ISPAIR(pattern) 
			&& ISSTR(CAR(expr))
			&& VARSTREQ(CAR(expr), "APPLY")) 
		return list_match(CDR(expr), pattern, t);
	return 0;
}

static int name_match(variant expr, variant pattern, VarTable** t) {
	if (ISSTR(expr) && VARSTREQ(expr, CAR(pattern).str)) return 1;
	return 0;
}

static int num_match(variant expr, variant pattern, VarTable** t) {
	if ( ISSTR(expr) 
			&& atof(expr.str) == atof(CAR(pattern).str))
		return 1;
	return 0;
}

static int var_match(variant expr, variant pattern, VarTable** t) {
	if ( ISPAIR(expr) && ISSTR(CAR(expr)) && VARSTREQ(CAR(expr), "VAR")) {
		/* Check if already unified */
		variant res;
		if ( vartable_get(*t, CAR(pattern).str, &res) ) return vareq(CADR(expr), res);
		vartable_set(t, CAR(pattern).str, CADR(expr));
		return 1;
	} 
	return 0;
}

static int const_match(variant expr, variant pattern, VarTable** t) {
	if ( ISPAIR(expr) && ISSTR(CAR(expr)) && VARSTREQ(CAR(expr), "NUM")) {
		variant res;
		if ( vartable_get(*t, CAR(pattern).str, &res) ) return atof(CADR(expr).str) == atof(res.str);
		vartable_set(t, CAR(pattern).str, CADR(expr));
		return 1;
	}
	return 0;
}

static int any_match(variant expr, variant pattern, VarTable** t) {
	variant res;
	if ( vartable_get(*t, CAR(pattern).str, &res) )  return vareq(expr, res);
	vartable_set(t, CAR(pattern).str, expr);
	return 1;
}

static int ellipsis_match(variant expr, variant pattern, VarTable** t) {
	/* should be called only from within list_match */
	return any_match(expr, pattern, t);
}

struct pattern_matcher {
	const char* name;
	int (*matchfunc)(variant, variant, VarTable** t);
};

struct pattern_matcher pmatchers[] = {
	{"LIST", list_match}, {"APPLY", apply_match}, {"NUM", num_match},
	{"NAME", name_match}, {"CONSTM", const_match}, {"VARM", var_match},
	{"ANYM", any_match}, {"ELLIPSIS", ellipsis_match}, {NULL, NULL}
};


int pattern_match(variant expr, variant ant, VarTable** t) {
	variant head = CAR(ant);
	if ( !ISSTR(head) ) return 0;
	for (struct pattern_matcher* m = pmatchers; m->name; m++ ) 
		if ( VARSTREQ(head, m->name) ) {
			
			//printf("Entering %s match of ", m->name);
			//varinline(expr);
			//printf(" and ");
			//varinline(CDR(ant));
			//putchar('\n');
		
			return m->matchfunc(expr, CDR(ant), t);
		}
	return 0;
}

int apply_rule(variant* expr, variant rule, variant rules) {
	int res = 0;
	variant ant = CADR(rule), conseq = CADDR(rule);

	VarTable* t = NULL;
	if (pattern_match(*expr, CADR(ant), &t)) {
		variant oldexpr = *expr;
		// debug 
		*expr = build_expr(CADR(conseq), t, rules);
		
		//printf("Application: \n");
		//varprint(*expr);
		//printf("---- \n");
		varfree(oldexpr);
		res = 1;
	}
	vartable_free(t);
	return res;
}

int apply_rules(variant *expr, variant rules) {
	for ( variant rule = CDR(rules); !ISNIL(rule); rule = CDR(rule)) 
		if (apply_rule(expr, CAR(rule), rules)) return 1;
	return 0;
}
