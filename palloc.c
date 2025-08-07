#include "alloc.h"

struct blk {
    size_t size;
    size_t mag_start;

    struct blk *nxt;
};

#define ALLOCED_MAG 0xD0DEC00L

struct alloc_t {
    void *pstart;

    size_t pool_size;

    struct blk *free_ls_hd;
};

typedef struct blk blk;
typedef struct alloc_t alloc_t;

static alloc_t alloc;

static unsigned char ppool[POOL_SIZE];

void pinit(void) {

    alloc.pstart = ppool;
    alloc.pool_size = POOL_SIZE;
    alloc.free_ls_hd = NULL;

    blk *init_blk = (blk *)alloc.pstart;

    init_blk->size = alloc.pool_size - sizeof(blk);
    init_blk->nxt = NULL;

    alloc.free_ls_hd = init_blk;

}

void *palloc(size_t size) {

    if (size == 0)
        return NULL;

    blk *cur = alloc.free_ls_hd;
    blk *prev = NULL;

    while (cur != NULL) {
        if (cur->size >= size) {
            if (cur->size > size + sizeof(blk)) {

                blk *new_blk = (blk *)((uint8_t *)cur + sizeof(blk) + size);
                new_blk->size = cur->size - size - sizeof(blk);
                new_blk->nxt = cur->nxt;

                if (prev == NULL)

                    alloc.free_ls_hd = new_blk;
                else
                    prev->nxt = new_blk;


                cur->size = size;

            } else {

                if (prev == NULL)
                    alloc.free_ls_hd = cur->nxt;
                else
                    prev->nxt = cur->nxt;

            }

            cur->mag_start = ALLOCED_MAG;
            uintptr_t end_mag_addr = (uintptr_t)((uint8_t*)cur + sizeof(blk) + cur->size);
            *(size_t*)end_mag_addr = ALLOCED_MAG;

            return (void *)((uint8_t *)cur + sizeof(blk));

        }

        prev = cur;
        cur = cur->nxt;
    }

    return NULL;
}

void pfree(void *p) {

    if (p == NULL || p < alloc.pstart || p >= (uint8_t *)alloc.pstart + alloc.pool_size)
        return;

    blk *blk_to_free = (blk *)((uint8_t *)p - sizeof(blk));

    uintptr_t end_mag_addr = (uintptr_t)((uint8_t*)p + blk_to_free->size);

    if (blk_to_free->mag_start != ALLOCED_MAG || *(size_t*)end_mag_addr != ALLOCED_MAG)
        return;

    blk_to_free->mag_start = 0;
    *(size_t*)end_mag_addr = 0;

    blk *cur = alloc.free_ls_hd,
        *prev = NULL;

    while (cur != NULL && cur < blk_to_free) {

        prev = cur;
        cur = cur->nxt;

    }

    if (prev == NULL) {

        blk_to_free->nxt = alloc.free_ls_hd;
        alloc.free_ls_hd = blk_to_free;

    } else {

        blk_to_free->nxt = prev->nxt;
        prev->nxt = blk_to_free;

    }

    if (blk_to_free->nxt != NULL && (uint8_t *)blk_to_free + sizeof(blk) + blk_to_free->size == (uint8_t *)blk_to_free->nxt) {

        blk_to_free->size += sizeof(blk) + blk_to_free->nxt->size;
        blk_to_free->nxt = blk_to_free->nxt->nxt;
    }

    if (prev != NULL && (uint8_t *)prev + sizeof(blk) + prev->size == (uint8_t *)blk_to_free) {

        prev->size += sizeof(blk) + blk_to_free->size;
        prev->nxt = blk_to_free->nxt;

    }

}

void *palloc_ali(size_t size, size_t alignment) {

    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        return NULL;

    size_t total_size = size + alignment + sizeof(void *);

    void *unaligned_ptr = palloc(total_size);

    if (unaligned_ptr == NULL)
        return NULL;

    uintptr_t aligned_addr = ((uintptr_t)unaligned_ptr + alignment + sizeof(void *) - 1) & ~(alignment - 1);

    void **ptr_to_og = (void **)(aligned_addr - sizeof(void *));

    memcpy(ptr_to_og, &unaligned_ptr, sizeof(void *));

    return (void *)aligned_addr;
}

void pfree_ali(void *p) {

    if (p == NULL)
        return;

    void **ptr_to_og = (void **)((uintptr_t)p - sizeof(void *));

    void *unaligned_ptr = *ptr_to_og;

    pfree(unaligned_ptr);

}