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

#endif
