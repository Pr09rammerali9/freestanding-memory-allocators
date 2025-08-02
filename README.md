# Minimalist Kernel And Freestanding Allocators

A suite of highly portable, high quallity memory allocators designed for use in freestanding environments such as operating system kernels, bootloaders, and embedded systems. This project provides a robust, self-contained solution for dynamic memory management without any external dependencies.

## Features

* **Two-Tiered Memory Management:** Includes a fast **bump allocator (`balloc`)** for simple, contiguous allocations, and a robust **first-fit pool allocator (`palloc`)** with coalescing to prevent memory fragmentation.
* **Extreme Portability:** The code relies solely on standard C primitives (`size_t`) and basic memory functions (`memcpy`, `memset`). It has no reliance on a standard library, language-specific runtime, or host OS.
* **Production-Grade Logic:** The `palloc` implementation includes crucial features like memory block coalescing and support for aligned allocations (`palloc_ali`), ensuring long-term stability and efficiency.
* **Freestanding Design:** The allocators can be used to manage memory in both a pre-paging and a post-paging environment, making them a versatile core component of any OS kernel.
* **Simple Interface:** The API is clean and easy to integrate, making it ideal for hobby OS development and educational purposes.

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
