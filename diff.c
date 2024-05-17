#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include "packrat.h"
#include "uforth.h"
#include "nlisp.h"
#include "pattern.h"


/* tree_builder builds the following LISP-like structure:
 * (EXPR (TERM (FACT (VAR x)) (BINOP *) (FACT (NUM 1.2))) (BINOP +) (TERM (FACT (VAR y) (POW ^) (VAR x)))) 
 * from parser tree to LISP:
 * pair - pair-------------------------
 *  |      |
 * EXPR   pair - pair ------------------------pair-------------------
 *         |      |                             | 
 *         TERM   pair - pair- NIL             pair-pair- NIL
 *                 |      |                     |    |
 *                FACT   pair -- pair - NIL   BINOP  *
 *                        |        |          
 *                        VAR     x  
 *
 * ((((VAR x)) (BINOP *) ((NUM 1.2))                       
 */

const char* atomnames[] = {"BINOP", "UNOP", "NAME", "NUM", "VAR", "POW", "ELLIPSIS", "CONST", "VARM", "ANYM", "CONSTM", NULL};

int tree_builder(Match* m, void* data) {
	const char** n;
	variant** ctx = (variant**)data;
	variant p = VARCONS(VARCONS(var_string(m->m->name), NIL), NIL);
	**ctx = p;
	*ctx = &CDR(p);
	for ( n  = atomnames; *n; n++ ) {
		if ( !strcmp(m->m->name, *n)) {
			CDAR(p) = VARCONS(var_string_from_range(m->start, m->end), NIL);
			return 0;
		}
	}
	variant* subctx = &CDAR(p);
	return match_walk(m, tree_builder, &subctx);

}

int match_to_expr_tree(Match* m, const char* start, variant* res) {
	*res = VARCONS(var_string(start), NIL);
	variant* ctx = &CDR(*res);
	return match_walk(m, tree_builder, &ctx);
}

int load_rules_and_apply(Grammar* gr, const char* rules, variant expr) {
	Match* mr = grammar_parse(gr, "RULES", rules);
	if ( !mr ) return -printf("Cannot parse rules\n");
	variant gres = NIL;
	match_to_expr_tree(mr, "RULES", &gres);
	varprint(gres);
	printf("Before application: \n");
	varprint(expr);
	apply_rules(&expr, gres);
	printf("Result of application: \n");
	varprint(expr);
	varfree(gres);
	varfree(expr);
	return 0;
}


int transform_with_rules(variant expr, const char* rule_file) {
	Grammar* gr = grammar_uforth_load("rules.ppf");
	if ( !gr ) return -printf("Cannot assemble rule grammar\n");

	FILE* dfr = fopen(rule_file, "r");
	if ( dfr ) {
		fseek(dfr, 0, SEEK_END);
		long sz = ftell(dfr);
		char* rules = calloc(sz+1,1);
		fseek(dfr, 0, SEEK_SET);
		fread(rules, sz, 1, dfr);
		fclose(dfr);
		load_rules_and_apply(gr, rules, expr);
		free(rules);
	}
	grammar_free(gr);
	return 0;
}


int main(int argc, char* argv[]) {
	Grammar* g = grammar_uforth_load("expr.ppf");
	variant result = NIL;
	if (!g ) {
		printf("Cannot assemble grammar\n");
		return -1;
	}
	printf("Parsing expression...\n");
	Match* m = grammar_parse(g, "EXPR",  argv[1]);
	if ( m ) {
		printf("Starting transformation...\n");
		match_to_expr_tree(m, "EXPR",  &result);
		varprint(result);
		transform_with_rules(result, "diffrule.rul");
	} else 
		printf("Cannot parse expression\n");
	grammar_free(g);
	return 0;
}
