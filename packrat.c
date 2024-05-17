#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "packrat.h"
#include "hash.h"

struct _grammar {
	Rule* rules;
	Matcher* allmatchers;
	int allocated;
};

size_t match_hash(const void* k) {
	size_t h = 0x919faed761fe119b;
	for(const char* x=(const char*)&k; x<(const char*)&k+sizeof(k); x++ ) {
		h += (*x)*0x98a8f3271fe5bcd;
		h = (h >> 43)|(h<< 21);
	}
	return h;
}

int match_cmp(const void* a, const void* b) {
	return a == b;
}

void _match_free(Table* t, void* k, void* a);

Rule* find_rule(Grammar* g, const char* name) {
	Rule* r;
	for( r = g->rules; r->name; r++ ) 
		if (!strcmp(r->name, name)) return r;
	return NULL;
}

Match* new_match(struct _matcher* m, const char* start, const char* end, Match* l, Match* r) {
	Match* n = malloc(sizeof(Match));
	*n = (Match){m, start, end, l, r};
	return n;
}
int take_two(Matcher* x, Matcher** stack) {
	if (!(*stack) ||  !(*stack)->next) return -1;
	x->left = (*stack)->next;
	x->right = (*stack);
	x->next = (*stack)->next->next;
	*stack = x;
	return 0;
}	

int take_one(Matcher* x, Matcher** stack) {
	if( !(*stack) ) return -1;
	x->left = (*stack);
	x->next = (*stack)->next;
	*stack = x;
	return 0;
}
int push(Matcher* x, Matcher** stack) {
	x->next = *stack;
	*stack = x;
	return 0;
}
static int cnt = 0;

/* match wrapper with auto cache */
Match* do_match(Matcher* m, const char* txt) {
	cnt ++;
	Match* res;
	//printf("Trying to parse \"%.10s\" with matcher %p(%d)\n", txt, m, m->n);
	if ( ht_get(m->cache, (void*)txt, (void**)&res) ) return res;
	res = m->func(m, txt);
	ht_set(m->cache, (void*)txt, res, res && res->m == m?_match_free:NULL);
	return res;
}

Match* seq_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	if ( ml ) {
		Match* mr = do_match(m->right, ml->end);
		if ( mr ) return new_match(m, txt, mr->end, ml, mr);
	} 
	return NULL;
}

Match* or_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	return ml ? ml : do_match(m->right, txt);
}

Match* not_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	return  ml ? NULL : new_match(m, txt, txt, NULL, NULL);
}

Match* optional_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	return ml ? ml : new_match(m, txt, txt, NULL, NULL);
}

Match* literal_match(Matcher* m, const char* txt) {
	return (!strncmp(txt, m->opt, strlen(m->opt)))? new_match(m, txt, txt+strlen(m->opt), NULL, NULL) : NULL;
}

Match* oneof_match(Matcher* m, const char* txt) {
	for ( const char* x = m->opt; *x; x++ ) 
		if (*txt == *x ) return new_match(m, txt, txt+1, NULL, NULL);
	return NULL;
}

Match* ahead_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	return ml ? new_match(m, txt, txt, NULL, NULL) : NULL;
}

Match* range_match(Matcher* m, const char* txt) {
	return (*txt >= m->opt[0] && *txt <= m->opt[1] ) ? new_match(m, txt, txt+1, NULL, NULL) : NULL;
}

Match* any_match(Matcher* m, const char* txt) {
	return  (*txt) ? new_match(m, txt, txt+1, NULL, NULL) : NULL;
}

Match* repeat_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	if ( ml ) {
		Match* mr = do_match(m, ml->end);
		if ( mr ) return new_match(m, txt, mr->end, ml, mr);
	}
	return new_match(m, txt, txt, NULL, NULL);
}

Match* ref_match(Matcher* m, const char* txt) {
	Rule* ref = m->ref;
	return ref ? do_match(ref->assembly, txt) : NULL;
}

Match* named_match(Matcher* m, const char* txt) {
	Match* ml = do_match(m->left, txt);
	return ml ? new_match(m, txt, ml->end, ml, NULL) : NULL;
}

int assemble_matcher(Grammar* g, Rule* r) {
	Matcher* stack = NULL;
	for (Matcher* x = r->matchers; x->func; x++ ) {
		switch(x->args) {
		case 2:
			if (take_two(x, &stack)) return !printf("Cannot build sequence -- nothing on stack\n");
			break;
		case 1:
			if ( take_one(x, &stack) ) return !printf("Cannot build not\n");
			break;
		case 0:
			if (x->func == ref_match) {
				x->ref = find_rule(g, x->opt);
				if ( !x->ref ) return !printf("Rule with name %s was not found in the grammar\n", x->opt);
			}
			push(x, &stack);
		default:
			break;
		}
		x->g = g; /* set ref to g */
		//x->rule = r; /* this rule reference */
		x->cache = ht_init(match_hash, match_cmp, NULL, NULL, 16, x);
	}
	if ( stack ){
		r->assembly = stack;
		return 1;
	}
	printf("Stack is empty for rule %s\n", r->name);
	return 0;
}

Grammar* assemble_grammar(Rule* rules, Matcher* matchers, int allocated) {
	Grammar* g = malloc(sizeof(Grammar));
	*g = (Grammar){rules, matchers, allocated};
	for ( Rule* r = rules; r->name; r++ ) 
		if (!assemble_matcher(g, r))  return (grammar_free(g), NULL);
	return g;
}

Match* grammar_parse(Grammar* g, const char* name, const char* txt) {
	Rule* r = find_rule(g, name);
	if( !r ) return (printf("Cannot find rule %s\n", name), NULL);
	Match* res = do_match(r->assembly, txt);
	printf("Matching took %d steps (%f steps per sym)\n", cnt, (float)cnt/(float)strlen(txt));
	return res;
}

void grammar_free(Grammar* g) {
	/* all matches are in cache */
	for (Rule*  r = g->rules; r->name; r++ ) {
		Matcher* m;
		for(m = r->matchers; m->func; m++) {
			if ( m->name && g->allocated) free(m->name);
			if ( m->opt && g->allocated) free(m->opt);
			if ( m->cache ) ht_free(m->cache);
		}
		if ( g->allocated) free(r->name);
	}
	if ( g->allocated) {
		free(g->rules);
		free(g->allmatchers);
	}
	free(g);
}

int _match_walk(Match* m, matchcb cb, void* data);

int match_walk(Match* m, matchcb cb, void* data) {
	if ( !m ) return 0;
	return _match_walk(m->left, cb, data) || _match_walk(m->right, cb, data);
}

int _match_walk(Match* m, matchcb cb, void* data) {
	if ( !m ) return 0;
	if ( m->m->name ) { 
		return cb(m, data)?1:0;
	} else
		return match_walk(m, cb, data);
}

void match_free(Match* m) {
	if ( !m ) return;
	match_free(m->left);
	match_free(m->right);
	free(m);
}

void _match_free(Table* t, void* k, void* a) {
	Match*  m = (Match*)a;
	if ( !m ) return;
	if ( m->m != ht_get_data(t)) {
		printf("Matcher %p is not the same as %p, table %p\n", m->m, ht_get_data(t), t);
		return;
	}
	free(a);
}
