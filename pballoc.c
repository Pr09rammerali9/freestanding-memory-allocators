#include "alloc.h"

#define PB_ALLOCED_MAG 0xC0DEBEEF
#define PB_POOL_SIZE 6451

typedef struct blk {
    size_t size;
    size_t mag;

    struct blk *nxt;
} blk;

typedef struct {
    void *pstart;
    size_t pool_size;
                                                      blk *free_ls_hd;
} pb_alloc_t;

static pb_alloc_t pb_alloc;
static unsigned char pb_pool[PB_POOL_SIZE];

void pbinit(void) {

    pb_alloc.pstart = pb_pool;
    pb_alloc.pool_size = PB_POOL_SIZE;                pb_alloc.free_ls_hd = NULL;                       blk *init_blk = (blk *)pb_alloc.pstart;
    init_blk->size = pb_alloc.pool_size - sizeof(blk);
    init_blk->nxt = NULL;
    pb_alloc.free_ls_hd = init_blk;

}

void *pballoc(size_t size) {

    if (size == 0)
        return NULL;

    blk *cur = pb_alloc.free_ls_hd;
    blk *prev = NULL;
    blk *best_fit_blk = NULL;
    blk *prev_best_fit = NULL;

    size_t smallest_size = (size_t)-1;

    while (cur != NULL) {

        if (cur->size >= size) {

            if (cur->size < smallest_size) {

                smallest_size = cur->size;
                best_fit_blk = cur;
                prev_best_fit = prev;

            }
        }

        prev = cur;
        cur = cur->nxt;

    }

    if (best_fit_blk == NULL)
        return NULL;

    if (best_fit_blk->size > size + sizeof(blk)) {

        blk *new_blk = (blk *)((uint8_t *)best_fit_blk + sizeof(blk) + size);
        new_blk->size = best_fit_blk->size - size - sizeof(blk);
        new_blk->nxt = best_fit_blk->nxt;

        if (prev_best_fit == NULL)
            pb_alloc.free_ls_hd = new_blk;
        else
            prev_best_fit->nxt = new_blk;

        best_fit_blk->size = size;

    } else {

        if (prev_best_fit == NULL)
            pb_alloc.free_ls_hd = best_fit_blk->nxt;
        else
            prev_best_fit->nxt = best_fit_blk->nxt;
    }

    best_fit_blk->mag = PB_ALLOCED_MAG;

    return (void *)((uint8_t *)best_fit_blk + sizeof(blk));
}

void pbfree(void *p) {

    if (p == NULL || p < pb_alloc.pstart || p >= (uint8_t *)pb_alloc.pstart + pb_alloc.pool_size)
        return;

    blk *blk_to_free = (blk *)((uint8_t *)p - sizeof(blk));

    if (blk_to_free->mag != PB_ALLOCED_MAG)
        return;

    blk_to_free->mag = 0;

    blk *cur = pb_alloc.free_ls_hd;
    blk *prev = NULL;

    while (cur != NULL && cur < blk_to_free) {

        prev = cur;
        cur = cur->nxt;

    }

    if (prev == NULL) {

        blk_to_free->nxt = pb_alloc.free_ls_hd;
        pb_alloc.free_ls_hd = blk_to_free;

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