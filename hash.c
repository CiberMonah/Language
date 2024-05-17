#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include "hash.h"

typedef struct _htelem {
	void* k;
	void* v;
	freefunc ff;
	struct _htelem* next;
} HTElem;

struct _table {
	hashfunc hf;
	cmpfunc cf;
	dupfunc df;
	freefunc ff;
	size_t elems;
	size_t sz;
	void *data;
	HTElem** table;
};

Table* ht_init(hashfunc hf, cmpfunc cf, dupfunc df, freefunc ff, size_t sz, void* data) {
	Table* t = malloc(sizeof(Table));
	*t = (Table){hf, cf, df, ff, 0, sz, data, calloc(sz, sizeof(HTElem*))};
	return t;
}

void* ht_get_data(Table* t) {
	return t->data;
}

static HTElem** _move(HTElem** t, HTElem** e) {
	HTElem* next = (*e)->next;
	if ( *e != *t ) { 
		(*e)->next = *t;
		*t = *e;
		*e = next;
		return e;
	}
	return &(*e)->next;
}

static int _rehash(Table* t) {
	size_t sz = t->sz*sizeof(HTElem*);
	if ( t->elems*10 < t->sz*9 ) return 0;
	t->table = realloc(t->table, 2*sz);
	memset(t->table + t->sz, 0, sz);
	for ( size_t i = 0; i < t->sz; i++) 
		for( HTElem** e = &t->table[i]; *e; e = _move(&t->table[t->hf((*e)->k) % (t->sz*2)], e));
	t->sz *= 2;
	return 1;
}
				
int ht_set(Table* t, void* k, const void* v, freefunc ff) {
	HTElem* e;
	size_t h = t->hf(k) % t->sz;
	for(e = t->table[h]; e; e=e->next) {
		if ( t->cf(k, e->k) ) {
			if ( e->ff ) e->ff(t, e->k, e->v); else if ( t->ff) t->ff(t, e->k, e->v);
			e->v = t->df?t->df(v):v;
			return 2;
		}
	}
	_rehash(t);
	e = malloc(sizeof(HTElem));
	*e = (HTElem){k, t->df?t->df(v):v, ff, t->table[h]};
	t->table[h] = e;
	t->elems++;
	return 1;
}

int ht_get(Table* t, const void* key, void** res) {
	size_t h = t->hf(key) % t->sz;
	for ( HTElem* e = t->table[h]; e; e = e->next ) 
		if ( t->cf(key, e->k) ) return (*res = e->v, 1);
	return 0;
}

void ht_free(Table* t) {
	HTElem* e, *n;
	for(size_t i = 0; i < t->sz; i++ ) {
		for(e = t->table[i]; e;) {
			n = e->next;
			if ( e->ff ) e->ff(t, e->k, e->v); else if ( t->ff ) t->ff(t, e->k, e->v);
			free(e);
			e = n;
		}
	}
	free(t->table);
	free(t);
}
