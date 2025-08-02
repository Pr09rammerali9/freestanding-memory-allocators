/*didnt have time + was writing fast, hope u understand, so expect some readabllity issues*/
#include "alloc.h"

#define TLSF_FL_INDEX_COUNT 32
#define TLSF_SL_INDEX_COUNT 32
#define TLSF_MAX_BLOCK_SIZE (1 << (TLSF_FL_INDEX_COUNT - 1))

#define POOL_SIZE_BYTES 7168
static char memory_pool[POOL_SIZE_BYTES];

#define MIN_BLOCK_SIZE (sizeof(blk_hdr_t))

typedef struct block_header {
    size_t size;
    struct block_header* next_free;
    struct block_header* prev_free;
    struct block_header* prev_block;
    struct block_header* next_block;
} blk_hdr_t;

typedef struct tlsf_t {
    blk_hdr_t* fl_list[TLSF_FL_INDEX_COUNT];
    blk_hdr_t* sl_list[TLSF_SL_INDEX_COUNT][TLSF_SL_INDEX_COUNT];
    uint32_t fl_bitmap;
    uint32_t sl_bitmap[TLSF_FL_INDEX_COUNT];
} tlsf_t;

static tlsf_t tlsf_allocator;

static void (*_tlsf_lock)(void) = NULL;
static void (*_tlsf_unlock)(void) = NULL;

static inline int find_free_ls(size_t size, int* fl, int* sl) {
    if (size == 0)
        return -1;
    *fl = (sizeof(size_t) * 8 - 1) - __builtin_clzll(size);
    *sl = (size >> (*fl - 1)) & (TLSF_SL_INDEX_COUNT - 1);
    return 0;
}

static void add_blk_to_free_ls(blk_hdr_t* blk) {
    int fl, sl;
    find_free_ls(blk->size, &fl, &sl);

    blk->next_free = tlsf_allocator.sl_list[fl][sl];
    if (tlsf_allocator.sl_list[fl][sl] != NULL)
        tlsf_allocator.sl_list[fl][sl]->prev_free = blk;
    blk->prev_free = NULL;
    tlsf_allocator.sl_list[fl][sl] = blk;

    tlsf_allocator.fl_bitmap |= (1 << fl);
    tlsf_allocator.sl_bitmap[fl] |= (1 << sl);
}

static void rm_blk_from_free_ls(blk_hdr_t* blk) {
    int fl, sl;
    find_free_ls(blk->size, &fl, &sl);

    if (blk->prev_free)
        blk->prev_free->next_free = blk->next_free;
    else
        tlsf_allocator.sl_list[fl][sl] = blk->next_free;
    if (blk->next_free)
        blk->next_free->prev_free = blk->prev_free;
    
    if (tlsf_allocator.sl_list[fl][sl] == NULL) {
        tlsf_allocator.sl_bitmap[fl] &= ~(1 << sl);
        if (tlsf_allocator.sl_bitmap[fl] == 0)
            tlsf_allocator.fl_bitmap &= ~(1 << fl);
    }
}

static inline blk_hdr_t* split_blk(blk_hdr_t* blk, size_t split_size) {
    size_t leftover_size = blk->size - split_size;
    blk->size = split_size;

    blk_hdr_t* leftover_blk = (blk_hdr_t*)((char*)blk + split_size);
    leftover_blk->size = leftover_size;

    leftover_blk->prev_block = blk;
    leftover_blk->next_block = blk->next_block;
    if (blk->next_block)
        blk->next_block->prev_block = leftover_blk;
    blk->next_block = leftover_blk;

    return leftover_blk;
}

static inline blk_hdr_t* coalesce_blk(blk_hdr_t* blk) {
    blk_hdr_t* prev_blk = blk->prev_block;
    if (prev_blk != NULL && prev_blk->next_free != NULL) {
        rm_blk_from_free_ls(prev_blk);
        blk->size += prev_blk->size;
        blk->prev_block = prev_blk->prev_block;
    }

    blk_hdr_t* next_blk = blk->next_block;
    if (next_blk != NULL && next_blk->next_free != NULL) {
        rm_blk_from_free_ls(next_blk);
        blk->size += next_blk->size;
        blk->next_block = next_blk->next_block;
        if (blk->next_block)
            blk->next_block->prev_block = blk;
    }
    return blk;
}

void tlsf_add_pool(void* mem_start, size_t mem_size) {
    if (mem_size < MIN_BLOCK_SIZE)
        return;

    blk_hdr_t* initial_blk = (blk_hdr_t*)mem_start;
    initial_blk->size = mem_size;
    initial_blk->prev_block = NULL;
    initial_blk->next_block = NULL;
    add_blk_to_free_ls(initial_blk);
}

void tlsf_init() {
    for (int i = 0; i < TLSF_FL_INDEX_COUNT; ++i) {
        tlsf_allocator.fl_list[i] = NULL;
        for (int j = 0; j < TLSF_SL_INDEX_COUNT; ++j) {
            tlsf_allocator.sl_list[i][j] = NULL;
        }
    }
    tlsf_allocator.fl_bitmap = 0;
    for (int i = 0; i < TLSF_FL_INDEX_COUNT; ++i) {
        tlsf_allocator.sl_bitmap[i] = 0;
    }
    
    tlsf_add_pool(memory_pool, POOL_SIZE_BYTES);
}

void* tlsf_alloc(size_t size) {
    if (size == 0)
        return NULL;
    
    size_t adjusted_size = (size + sizeof(blk_hdr_t) + (__alignof__(max_align_t) - 1)) & ~(__alignof__(max_align_t) - 1);
    if (adjusted_size < MIN_BLOCK_SIZE)
        adjusted_size = MIN_BLOCK_SIZE;
    
    int fl, sl;
    find_free_ls(adjusted_size, &fl, &sl);

    int fl_index = fl;
    uint32_t fl_mask = tlsf_allocator.fl_bitmap & (~0U << fl);
    if (fl_mask == 0)
        return NULL;
    fl_index = __builtin_ffs(fl_mask) - 1;

    int sl_index = sl;
    uint32_t sl_mask = tlsf_allocator.sl_bitmap[fl_index] & (~0U << sl);
    if (sl_mask == 0) {
        fl_index = __builtin_ffs(fl_mask & (~0U << (fl_index + 1))) - 1;
        sl_index = __builtin_ffs(tlsf_allocator.sl_bitmap[fl_index]) - 1;
    } else
        sl_index = __builtin_ffs(sl_mask) - 1;

    blk_hdr_t* blk = tlsf_allocator.sl_list[fl_index][sl_index];
    rm_blk_from_free_ls(blk);

    if (blk->size > adjusted_size) {
        blk_hdr_t* leftover_blk = split_blk(blk, adjusted_size);
        add_blk_to_free_ls(leftover_blk);
    }
    
    return (void*)((char*)blk + sizeof(blk_hdr_t));
}

void* tlsf_alloc_ali(size_t size, size_t alignment) {
    if (size == 0)
        return NULL;
    
    size_t extra_padding = alignment - 1;
    size_t required_size = size + extra_padding + sizeof(blk_hdr_t);

    int fl, sl;
    find_free_ls(required_size, &fl, &sl);

    int fl_index = fl;
    uint32_t fl_mask = tlsf_allocator.fl_bitmap & (~0U << fl);
    if (fl_mask == 0)
        return NULL;
    fl_index = __builtin_ffs(fl_mask) - 1;

    int sl_index = sl;
    uint32_t sl_mask = tlsf_allocator.sl_bitmap[fl_index] & (~0U << sl);
    if (sl_mask == 0) {
        fl_index = __builtin_ffs(fl_mask & (~0U << (fl_index + 1))) - 1;
        sl_index = __builtin_ffs(tlsf_allocator.sl_bitmap[fl_index]) - 1;
    } else
        sl_index = __builtin_ffs(sl_mask) - 1;

    blk_hdr_t* blk = tlsf_allocator.sl_list[fl_index][sl_index];
    rm_blk_from_free_ls(blk);

    char* aligned_ptr = (char*)((((uintptr_t)blk + sizeof(blk_hdr_t)) + alignment - 1) & ~(alignment - 1));
    size_t aligned_blk_size = size + (aligned_ptr - (char*)blk);

    if (blk->size > aligned_blk_size) {
        size_t leftover_size = blk->size - aligned_blk_size;
        blk->size = aligned_blk_size;

        blk_hdr_t* leftover_blk = (blk_hdr_t*)((char*)blk + aligned_blk_size);
        leftover_blk->size = leftover_size;

        leftover_blk->prev_block = blk;
        leftover_blk->next_block = blk->next_block;
        if (blk->next_block)
            blk->next_block->prev_block = leftover_blk;
        blk->next_block = leftover_blk;

        add_blk_to_free_ls(leftover_blk);
    }

    return (void*)aligned_ptr;
}

void tlsf_free(void* p) {
    if (p == NULL)
        return;

    blk_hdr_t* blk = (blk_hdr_t*)((char*)p - sizeof(blk_hdr_t));
    blk_hdr_t* merged_blk = coalesce_blk(blk);
    add_blk_to_free_ls(merged_blk);
}

void tlsf_init_lk(void (*lk)(void), void (*unlk)(void)) {
    _tlsf_lock = lk;
    _tlsf_unlock = unlk;
    tlsf_init();
}

void* tlsf_alloc_lk(size_t size) {
    if (_tlsf_lock)
        _tlsf_lock();
    void* p = tlsf_alloc(size);
    if (_tlsf_unlock)
        _tlsf_unlock();
    return p;
}

void* tlsf_alloc_ali_lk(size_t size, size_t alignment) {
    if (_tlsf_lock)
        _tlsf_lock();
    void* p = tlsf_alloc_ali(size, alignment);
    if (_tlsf_unlock)
        _tlsf_unlock();
    return p;
}

void tlsf_free_lk(void* p) {
    if (_tlsf_lock)
        _tlsf_lock();
    tlsf_free(p);
    if (_tlsf_unlock)
        _tlsf_unlock();
}
