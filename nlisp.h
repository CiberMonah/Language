#ifndef _NLISP_H_
#define _NLISP_H_

#define VAR_NIL 0
#define VAR_PAIR 1
#define VAR_STR 2

typedef struct _pair pair;

typedef struct _variant variant; 

struct _variant {
	int type;
	union {
		pair* pair;
		char* str;
	};
};
struct _pair {
	variant head;
	variant tail;
};


#define NIL (variant){VAR_NIL, NULL}
#define ISNIL(x) ((x).type == VAR_NIL)
#define ISPAIR(x) ((x).type == VAR_PAIR)
#define ISSTR(x) ((x).type == VAR_STR)
variant var_string_from_range(const char* start, const char* end);

variant var_string(const char* s);


pair* cons(variant head, variant tail);

#define VARCONS(head, tail) (variant){VAR_PAIR, cons(head, tail)}
#define CAR(x) ((x).pair->head)
#define CDR(x) ((x).pair->tail)
#define CDAR(x) ((x).pair->head.pair->tail)
#define CADR(x) ((x).pair->tail.pair->head)
#define CADDR(x) CADR(CDR(x))
#define VARSTREQ(x, s) !strcmp((x).str, s)

void varprint(variant x);

void varinline(variant x);

void varprintlist(variant lst);

void varfree(variant x);

int vareq(variant a, variant b);

variant varclone(variant x);

#endif /*_NLISP_H_ */
