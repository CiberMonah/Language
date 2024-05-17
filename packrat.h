#ifndef _PACKRAT_H_
#define _PACKRAT_H_
#include "hash.h"

/** base matcher */
typedef struct _matcher {
	char* name;
	struct _match* (*func)(struct _matcher*, const char*);
	int args;
	char* opt;
	int n;
	struct _matcher* left;
	struct _matcher* right;
	struct _rule* ref;
	struct _grammar* g;
	struct _matcher* next;
	Table* cache;
} Matcher;

typedef struct _match {
	Matcher* m;
	const char* start;
	const char* end;
	struct _match* left;
	struct _match* right;
} Match;

typedef struct _rule {
	char* name;
	Matcher* matchers;
	Matcher* assembly;
} Rule;



typedef struct _grammar Grammar; 
typedef int (*matchcb)(Match* m, void* data);

int match_walk(Match* x, matchcb cb, void* data);
void match_free(Match* m);


Match* seq_match(Matcher*, const char*);
Match* or_match(Matcher* m, const char* txt);
Match* not_match(Matcher* m, const char* txt);
Match* optional_match(Matcher* m, const char* txt);
Match* literal_match(Matcher* m, const char* txt);
Match* oneof_match(Matcher* m, const char* txt);
Match* ahead_match(Matcher* m, const char* txt);
Match* range_match(Matcher* m, const char* txt);
Match* any_match(Matcher* m, const char* txt);
Match* repeat_match(Matcher* m, const char* txt);
Match* ref_match(Matcher* m, const char* txt);
Match* named_match(Matcher* m, const char* txt);

Grammar* assemble_grammar(Rule* r, Matcher* m,  int allocated);
Match* grammar_parse(Grammar* g, const char* rulename, const char* txt);
/** Cleanup grammar resources */
void grammar_free(Grammar*);

#define MSEQ {NULL, seq_match, 2}
#define MOR  {NULL, or_match, 2}
#define NAMED_SEQ(name) {name, seq_match, 2}
#define MNOT {NULL, not_match, 1}
#define MOPT {NULL, optional_match, 1}
#define MLITERAL(s) {NULL, literal_match, 0, (s)}
#define MONEOF(s) {NULL, oneof_match, 0, (s)}
#define MAHEAD {NULL, ahead_match, 1}
#define MRANGE(a, b) {NULL, range_match, 0,  a b}
#define MANY {NULL, any_match, 0}
#define MREP {NULL, repeat_match, 1}
#define NAMED_MREP(s) {s, repeat_match, 1}
#define MREF(s) {NULL, ref_match, 0, (s)}
#define MNAMED(s) {(s), named_match, 1}
#define MEND {0}

#endif
