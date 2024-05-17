#ifndef _HASH_H_
#define _HASH_H_
#include<stdint.h>

typedef struct _table Table;
typedef uint64_t (*hashfunc)(const void*);
typedef int (*cmpfunc)(const void*, const void*);
typedef void* (*dupfunc)(const void*);
typedef void (*freefunc)(Table*, void*, void*);


Table* ht_init(hashfunc hf, cmpfunc cf, dupfunc df, freefunc ff, size_t sz, void* data);
int ht_set(Table* t, void* key, const void* v, freefunc ff);
int ht_get(Table* t, const void* key, void** res);
void* ht_get_data(Table* t);
void ht_free(Table* t);

#endif /* _HASH_H_ */
