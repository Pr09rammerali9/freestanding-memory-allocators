#ifndef ALLOC_H
#define ALLOC_H

#include "mem.h"
#include "def.h"
#include "bit/ptr.h"

#define alloca(n) __builtin_alloca(n)
/*a bumb allocator*/
#define POOL_SIZE 6656

/*b prefix for bumb*/

void *balloc(size_t size, size_t alignment);
void breset(void);

size_t bget_used(void);
size_t bget_remaining(void);

/*p prefix for pool*/
void pinit(void);
void *palloc(size_t size);
void *palloc_ali(size_t size, size_t alignment);
void pfree(void *p);

void tlsf_init(void);
void tlsf_init_lk(void (*lk)(void), void (*unlk)(void));
void *tlsf_alloc(size_t size);
void *tlsf_alloc_lk(size_t size);
void *tlsf_alloc_ali(size_t size, size_t alignment);
void *tlsf_alloc_ali_lk(size_t size, size_t alignment);
void tlsf_free(void *p);
void tlsf_free_lk(void *p);

#endif