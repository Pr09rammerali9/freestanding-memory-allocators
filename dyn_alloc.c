#include "alloc.h" 

#define TLSF_FL_INDEX_COUNT 32
#define TLSF_SL_INDEX_COUNT 32

#define TLSF_MAGIC 0xFEEDC0DE
#define MIN_BLOCK_SIZE (sizeof(blk_hdr_t) + sizeof(size_t))

typedef struct block_header {
    size_t size;
    size_t prev_size;
    struct block_header* next_free;
    struct block_header* prev_free;
} blk_hdr_t;

typedef struct tlsf_t {
    blk_hdr_t* sl_list[TLSF_FL_INDEX_COUNT][TLSF_SL_INDEX_COUNT];
    uint32_t fl_bitmap;
    uint32_t sl_bitmap[TLSF_FL_INDEX_COUNT];
    void (*fl_locks[TLSF_FL_INDEX_COUNT])(void);
    void (*fl_unlocks[TLSF_FL_INDEX_COUNT])(void);
} tlsf_t;

static tlsf_t tlsf_allocator;

static inline int find_free_ls(size_t size, int* fl, int* sl) {
    if (size == 0) return -1;
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

static inline blk_hdr_t* get_next_block(blk_hdr_t* blk) {
    return (blk_hdr_t*)((uintptr_t)blk + blk->size);
}

static inline blk_hdr_t* get_prev_block(blk_hdr_t* blk) {
    return (blk_hdr_t*)((uintptr_t)blk - blk->prev_size);
}

static inline blk_hdr_t* split_blk(blk_hdr_t* blk, size_t split_size) {
    blk_hdr_t* leftover_blk = (blk_hdr_t*)((uintptr_t)blk + split_size);
    leftover_blk->size = blk->size - split_size;
    leftover_blk->prev_size = split_size;
    blk->size = split_size;
    get_next_block(leftover_blk)->prev_size = leftover_blk->size;
    return leftover_blk;
}

static inline blk_hdr_t* coalesce_blk(blk_hdr_t* blk) {
    blk_hdr_t* prev_blk = (blk_hdr_t*)((uintptr_t)blk - blk->prev_size);
    if (blk->prev_size > 0 && prev_blk->size == blk->prev_size) {
        rm_blk_from_free_ls(prev_blk);
        blk->size += prev_blk->size;
        blk = prev_blk;
    }

    blk_hdr_t* next_blk = get_next_block(blk);
    if (next_blk->size > 0 && get_next_block(next_blk) == (blk_hdr_t*)((uintptr_t)blk + blk->size)) {
        rm_blk_from_free_ls(next_blk);
        blk->size += next_blk->size;
    }
    get_next_block(blk)->prev_size = blk->size;
    return blk;
}

static void tlsf_add_pool_internal(void* mem_start, size_t mem_size) {
    if (mem_size < MIN_BLOCK_SIZE) return;

    blk_hdr_t* initial_blk = (blk_hdr_t*)mem_start;
    initial_blk->size = mem_size - MIN_BLOCK_SIZE;
    initial_blk->prev_size = 0;
    add_blk_to_free_ls(initial_blk);
    get_next_block(initial_blk)->prev_size = initial_blk->size;
}

void dyn_init(void *pool, size_t size) {
    for (int i = 0; i < TLSF_FL_INDEX_COUNT; ++i) {
        for (int j = 0; j < TLSF_SL_INDEX_COUNT; ++j) {
            tlsf_allocator.sl_list[i][j] = NULL;
        }
        tlsf_allocator.fl_locks[i] = NULL;
        tlsf_allocator.fl_unlocks[i] = NULL;
    }
    tlsf_allocator.fl_bitmap = 0;
    for (int i = 0; i < TLSF_FL_INDEX_COUNT; ++i) {
        tlsf_allocator.sl_bitmap[i] = 0;
    }
    tlsf_add_pool_internal(pool, size);
}

void dyn_init_lk(void *pool, size_t size, void (*lk)(void), void (*unlk)(void)) {
    dyn_init(pool, size);
    for (int i = 0; i < TLSF_FL_INDEX_COUNT; ++i) {
        tlsf_allocator.fl_locks[i] = lk;
        tlsf_allocator.fl_unlocks[i] = unlk;
    }
}

void dyn_add_pool(void *pool, size_t size) {
    tlsf_add_pool_internal(pool, size);
}

void dyn_add_pool_lk(void *pool, size_t size) {
    int fl, sl;
    find_free_ls(size, &fl, &sl);

    if (tlsf_allocator.fl_locks[fl]) tlsf_allocator.fl_locks[fl]();
    tlsf_add_pool_internal(pool, size);
    if (tlsf_allocator.fl_unlocks[fl]) tlsf_allocator.fl_unlocks[fl]();
}

void *dyn_alloc(size_t size, size_t alignment) {
    if (size == 0) return NULL;

    size_t adjusted_size = (size + sizeof(blk_hdr_t) + (alignment - 1)) & ~(alignment - 1);
    if (adjusted_size < MIN_BLOCK_SIZE) adjusted_size = MIN_BLOCK_SIZE;

    int fl, sl;
    find_free_ls(adjusted_size, &fl, &sl);

    int fl_index = fl;
    uint32_t fl_mask = tlsf_allocator.fl_bitmap & (~0U << fl);
    if (fl_mask == 0) return NULL;
    fl_index = __builtin_ffs(fl_mask) - 1;

    int sl_index = sl;
    uint32_t sl_mask = tlsf_allocator.sl_bitmap[fl_index] & (~0U << sl);
    if (sl_mask == 0) {
        fl_index = __builtin_ffs(fl_mask & (~0U << (fl_index + 1))) - 1;
        sl_index = __builtin_ffs(tlsf_allocator.sl_bitmap[fl_index]) - 1;
    } else sl_index = __builtin_ffs(sl_mask) - 1;

    if (tlsf_allocator.fl_locks[fl_index]) tlsf_allocator.fl_locks[fl_index]();

    blk_hdr_t* blk = tlsf_allocator.sl_list[fl_index][sl_index];
    rm_blk_from_free_ls(blk);

    if (blk->size >= adjusted_size + MIN_BLOCK_SIZE) {
        blk_hdr_t* leftover_blk = split_blk(blk, adjusted_size);
        add_blk_to_free_ls(leftover_blk);
    } else {
        adjusted_size = blk->size;
    }

    if (tlsf_allocator.fl_unlocks[fl_index]) tlsf_allocator.fl_unlocks[fl_index]();

    return (void*)((char*)blk + sizeof(blk_hdr_t));
}

void *dyn_alloc_lk(size_t size, size_t alignment) {
    return dyn_alloc(size, alignment);
}

void dyn_free(void* p) {
    if (p == NULL) return;

    blk_hdr_t* blk = (blk_hdr_t*)((char*)p - sizeof(blk_hdr_t));

    int fl, sl;
    find_free_ls(blk->size, &fl, &sl);

    if (tlsf_allocator.fl_locks[fl]) tlsf_allocator.fl_locks[fl]();

    blk_hdr_t* merged_blk = coalesce_blk(blk);
    add_blk_to_free_ls(merged_blk);

    if (tlsf_allocator.fl_unlocks[fl]) tlsf_allocator.fl_unlocks[fl]();
}

void dyn_free_lk(void* p) {
    if (p == NULL) return;

    blk_hdr_t* blk = (blk_hdr_t*)((char*)p - sizeof(blk_hdr_t));

    int fl, sl;
    find_free_ls(blk->size, &fl, &sl);

    if (tlsf_allocator.fl_locks[fl]) tlsf_allocator.fl_locks[fl]();
    
    blk_hdr_t* merged_blk = coalesce_blk(blk);
    add_blk_to_free_ls(merged_blk);

    if (tlsf_allocator.fl_unlocks[fl]) tlsf_allocator.fl_unlocks[fl]();
}
