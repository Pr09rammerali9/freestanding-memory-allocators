(Check UPD.md for updates)
# Minimalist Kernel And Freestanding Allocators
#### (very important note:the best fit pool allocators are not for general purpoes use, they are just incase allocators, they dont offer much safety) 

A suite of highly portable, high quallity memory allocators designed for use in freestanding environments such as operating system kernels, bootloaders, and embedded systems. This project provides a robust, self-contained solution for dynamic memory management without any external dependencies.

## Features

* **Two-Tiered Memory Management:** Includes a fast **bump allocator (`balloc`)** for simple, contiguous allocations, and a robust **first-fit pool allocator (`palloc`)** with coalescing to prevent memory fragmentation.
* **Extreme Portability:** The code relies solely on standard C primitives (`size_t`) and basic memory functions (`memcpy`, `memset`). It has no reliance on a standard library, language-specific runtime, or host OS.
* **Production-Grade Logic:** The `palloc` implementation includes crucial features like memory block coalescing and support for aligned allocations (`palloc_ali`), ensuring long-term stability and efficiency.
* **Freestanding Design:** The allocators can be used to manage memory in both a pre-paging and a post-paging environment, making them a versatile core component of any OS kernel.
* **Simple Interface:** The API is clean and easy to integrate, making it ideal for hobby OS development and educational purposes.
* **Minimal:** it only uses math, pointer arthicmetic, memcpy, memcmp, memset
* **efficiency:** its only few hundreds of lines in total
## Usage

### `balloc` (Bump Allocator)

`balloc` is for fast, sequential memory allocation. It does not support individual frees.

```c
#include "alloc.h"

// Allocate a 100-byte block
void *block = balloc(100, 8); // 8-byte alignment

if (block) {
    // use block
}

// Reset the entire pool
breset(); 

```
### `palloc` (Pool Allocator)
`palloc` And `palloc_ali` is for longer lived objects, it supports individual frees

```c
#include "alloc.h"

// Initialize the pool allocator
pinit();

// Allocate a 100-byte block
void *p1 = palloc(100);

if (p1) {
    // use p1
    pfree(p1); // Free the block
}

// Allocate an aligned block for a specific purpose
void *p2 = palloc_ali(128, 64); // 64-byte alignment for a DMA buffer, for example
```

# Update:TLSF allocator added

### Features:
TLSF Allocator (tlsf_alloc)
The Two-Level Segregated Fit (TLSF) allocator provides a fast, O(1) solution for dynamic memory management. It is ideal for complex, multi-threaded environments where performance and low fragmentation are critical.
 * O(1) Time Complexity: Memory allocations and deallocations are performed in constant time, regardless of the pool size.
 * Thread Safety: The allocator can be initialized with user-provided lock and unlock functions to ensure safe operation in a multi-threaded context.
 * Dynamic Pools: The allocator's memory can be extended dynamically.
 * Aligned Allocations: Supports memory allocation with a specified alignment using tlsf_alloc_ali. 

Example:
```c
#include "alloc.h"
#include <stdio.h>

// Example lock and unlock functions for a multi-threaded context
void my_lock(int lk_n) {
    // ... acquire mutex ...
}

void my_unlock(int lk_n) {
    // ... release mutex ...
}

int main() {
    // Initialize the thread-safe allocator with our lock functions
    tlsf_init_lk(my_lock, my_unlock);

    // Allocate memory using the thread-safe function
    size_t data_size = 128;
    void* p1 = tlsf_alloc_lk(data_size);

    if (p1 != NULL) {
        printf("Allocated 128 bytes at address: %p\n", p1);
        // use the allocated memory...
    }

    // Allocate memory with a specific alignment (e.g., 64 bytes)
    void* p2 = tlsf_alloc_ali_lk(100, 64);
    if (p2 != NULL) {
        printf("Allocated 100 aligned bytes at address: %p\n", p2);
    }
    
    // Free the allocated memory
    tlsf_free_lk(p1);
    tlsf_free_lk(p2);

    return 0;
}
