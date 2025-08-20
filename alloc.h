#ifndef ALLOC_H
#define ALLOC_H

#include "mem.h"
#include <stddef.h>
#include <stdint.h>

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
void pfree_ali(void *p);
void pinit_lk(void (*lk)(void), void (*unlk)(void));
void *palloc_lk(size_t size);
void *palloc_ali_lk(size_t size, size_t alignment);
void pfree_ali_lk(void *p);

/*pb prefix for pool allocator with best fit algorthim*/
void pbinit(void);
void *pballoc(size_t size);
void pbfree(void *p);
void pbinit_lk(void (*lk)(void), void (*unlk)(void));
void *pballoc_lk(size_t size);
void pbfree_lk(void *p);

void tlsf_init();
void tlsf_add_pool(void* mem_start, size_t mem_size);
void* tlsf_alloc(size_t size);
void* tlsf_alloc_ali(size_t size, size_t alignment);
void tlsf_free(void* p);
void tlsf_init_lk(void (*lk)(int), void (*unlk)(int));
void tlsf_add_pool_lk(void* mem_start, size_t mem_size, int fl_index);
void* tlsf_alloc_lk(size_t size);
void* tlsf_alloc_ali_lk(size_t size, size_t alignment);
void tlsf_free_lk(void* p);


#endif