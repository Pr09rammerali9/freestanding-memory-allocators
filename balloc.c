#include "alloc.h"

static unsigned char pool[3072] = { 0 };

static size_t off = 0;

void *balloc(size_t size, size_t alignment) {

    if (!size) //size is zero, this is similar to if (size == 0)
        return NULL;

    uintptr_t cur_addr = (uintptr_t)pool + off;

    size_t padding = 0;

    if (alignment > 0) {

        size_t rem = cur_addr % alignment;

        if (rem != 0)
            padding = alignment - rem;
    }

    if (off + padding + size > 3072)
        return NULL;

    unsigned char *alloced_ptr = pool + off + padding;

    off += (padding + size);

    return (void *)alloced_ptr;

}

void breset(void) {

    off = 0;

    memset(pool, 0, 3072);

}

size_t bget_used(void) {

    return off;

}

size_t bget_remaining(void) {

    return 3072 - off;

} 
