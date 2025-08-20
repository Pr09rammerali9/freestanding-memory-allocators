### Added Pool Allocator With Best Fit Algorthim
Short Description:a Pool Allocator With coalescing And Best Fit Algorthim
Functions:
```c
void pbinit(void);
void *pballoc(size_t size);
void pbfree(void *p);
```
### update log 2: Added ``` pfree_ali(void *p) ``` For palloc_ali Function
*** fixed some bugs and added improvements ***

*** added tlsf_add_pool_lk for safety ***

### update log 4:redesigned tlsf allocator for more security and safety
new prototypes:

```c
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
```


*** add back original tlsf allocators before replaced as old_tlsf_*:

```c
void old_tlsf_add_pool(void* mem_start, size_t mem_size);
void old_tlsf_init();
void* old_tlsf_alloc(size_t size);
void* old_tlsf_alloc_ali(size_t size, size_t alignment);
void old_tlsf_free(void* p);
void old_tlsf_init_lk(void (*lk)(void), void (*unlk)(void));
void old_tlsf_add_pool_lk(void* mem_start, size_t mem_size);
void* old_tlsf_alloc_lk(size_t size);
void* old_tlsf_alloc_ali_lk(size_t size, size_t alignment);
void old_tlsf_free_lk(void* p);
```

