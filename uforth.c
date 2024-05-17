#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "packrat.h"

/* helpful macros */
#define ESC2SYM(c) ((c)=='t'?'\t':(c)=='n'?'\n':(c))
#define ISWHITE(c) ((c)==' '||(c)=='\n'||(c)=='\t')

/* simple tokenizer with support of escaping */
int _tokenize(FILE* f, char* buf, size_t bufsz, int waitfor, int escape) {
	int c = getc(f);
	if ( bufsz == 0 ) return -2; /* out of buffer */
	if (c == EOF)  return (*buf = 0, c); /* end of file */
	if ( escape ) return (*buf = ESC2SYM(c), _tokenize(f, buf+1, bufsz-1, waitfor, 0)); /* processing escape */
	else if ( waitfor && c == waitfor ) return (*buf = 0, waitfor); /* waitfor encountered */
	else if (!waitfor && ISWHITE(c)) return (*buf= 0, waitfor); /* whitespace */
	else if ( c == '\\' ) return _tokenize(f, buf, bufsz, waitfor, 1); /* entering escape mode */
	else return (*buf = c, _tokenize(f, buf+1, bufsz-1,  waitfor, 0)); /* continue filling buffer */
}
/* read the file token-by-token */
int tokenize(FILE* f, char* buf, size_t bufsz) {
	int c = getc(f);
	switch(c) {
	case EOF: return c;
	case ' ':  case '\t': case '\n': return tokenize(f, buf, bufsz);
	case '"': return _tokenize(f, buf, bufsz, '"', 0)=='"'?'"':EOF;
	case '{': return _tokenize(f, buf, bufsz, '}', 0)=='}'?'{':EOF;
	case '[': return _tokenize(f, buf, bufsz, ']', 0)==']'?'[':EOF;
	default: ungetc(c, f); return _tokenize(f, buf, bufsz, 0, 0)==0?buf[0]:EOF;
	}
}

Grammar* grammar_uforth_load(const char* name) {
	Grammar* g;
	size_t rules_sz =16;
	size_t matchers_sz = 256;
	FILE *f = fopen(name, "r");
	if ( !f ) return (fprintf(stderr, "Cannot open grammar uforth %s\n", name), NULL);
	Matcher* allmatchers = calloc(sizeof(Matcher), matchers_sz); 
	Rule* rules = calloc(sizeof(Rule), rules_sz);
	size_t bufsz = 1024;
	char buf[bufsz];
	int r = 0;
	int m = 0;
	int ttype;
	while(1) {
		ttype = tokenize(f, buf, bufsz);
		if ( ttype == EOF) break;
		if ( ttype == -2) {fprintf(stderr, "Out of buffer when reading rule %d, matcher %d at %lu\n", r, m, ftell(f)); break;}
		if ( r >= rules_sz-1 ) rules = realloc(rules, sizeof(Rule)*(rules_sz*=2));
		if ( m >= matchers_sz-1 ) allmatchers = realloc(allmatchers, sizeof(Matcher)*(matchers_sz*=2));
		switch(ttype) {
		case '%': if ( m > 0 ) allmatchers[m++] = (Matcher)MEND; rules[r++] = (Rule){strdup(buf+1), allmatchers+m}; break;
		case '"': allmatchers[m++] = (Matcher){NULL, literal_match, 0, strdup(buf), m}; break;
		case '{': allmatchers[m++] = (Matcher){NULL, range_match, 0, strdup(buf), m}; break;
		case '[': allmatchers[m++] = (Matcher){NULL, oneof_match ,0, strdup(buf), m}; break;
		case '?': allmatchers[m++] = (Matcher){NULL, optional_match, 1, .n=m}; break;
		case '*': allmatchers[m++] = (Matcher){NULL, repeat_match, 1, .n=m}; break;
		case '@': allmatchers[m++] = (Matcher){strdup(buf+1), named_match, 1, .n=m}; break;
		case '.': allmatchers[m++] = (Matcher){NULL, any_match, 0, .n=m}; break;
		case '|': allmatchers[m++] = (Matcher){NULL, or_match, 2, .n=m}; break;
		case ';': allmatchers[m++] = (Matcher){NULL, seq_match, 2, .n=m}; break;
		case '!': allmatchers[m++] = (Matcher){NULL, not_match, 1, .n=m}; break;
		case '&': allmatchers[m++] = (Matcher){NULL, ahead_match, 1, .n=m}; break;
		default:  allmatchers[m++] = (Matcher){NULL, ref_match, 0, strdup(buf), .n=m}; break;
		}
	}
	if (ttype < -1 ) {
		fclose(f);
		fprintf(stderr, "Error while parsing\n");
		free(rules);
		free(allmatchers);
		return NULL;
	}
	fclose(f);
	allmatchers[m++] = (Matcher)MEND;
	rules[r++] = (Rule){NULL, NULL};
	fprintf(stderr, "Rules found: %d, Matchers: %d\n", r, m);
	g = assemble_grammar(rules, allmatchers, 1);
	return g;
}
