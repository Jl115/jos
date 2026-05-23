# Phase 5: Dynamic Memory (Kernel Heap)

## What You're Building

A **kernel heap allocator** that provides `kmalloc()` and `kfree()` for variable-sized allocations inside the kernel. Unlike your PFA (Phase 3) which allocates whole 4 KiB pages, the heap allocator splits pages into smaller chunks.

---

## Core Concepts You Must Learn

### 1. The Kernel Heap vs The Page Frame Allocator

| Layer                          | Granularity                     | API                         | Purpose                                   |
| ------------------------------ | ------------------------------- | --------------------------- | ----------------------------------------- |
| **Page Frame Allocator (PFA)** | 4 KiB (fixed)                   | `pfa_alloc()`, `pfa_free()` | Physical memory at page level             |
| **Kernel Heap**                | Variable (8, 16, 32, ... bytes) | `kmalloc(sz)`, `kfree(ptr)` | Dynamic data structures inside the kernel |

Your heap **sits on top of** the PFA. When the heap needs more memory, it calls `pfa_alloc()` to get a fresh page. When `kfree()` returns the last used chunk in a page, the heap may return the page to the PFA.

### 2. Simple Fixed-Size Allocator (Freelist)

The simplest kernel heap approach is a **segregated freelist** with fixed-size bins.

#### Pseudocode structure

```
#define MIN_BLOCK_SIZE   16    // smallest allocation
#define MAX_BLOCK_SIZE   2048  // largest allocation before going directly to PFA
#define NUM_BINS         8     // sizes: 16, 32, 64, 128, 256, 512, 1024, 2048

typedef struct block_header {
    struct block_header *next;
} block_header_t;

typedef struct {
    block_header_t *free_list[NUM_BINS];  // one free list per size
} heap_t;

// Find which bin a size belongs to
size_t bin_index(size_t size):
    if size <= MIN_BLOCK_SIZE: return 0
    if size <= 32: return 1
    ...
```

#### Allocation (`kmalloc`)

```
void *kmalloc(size_t size):
    if size > MAX_BLOCK_SIZE:
        // Allocate whole pages via PFA
        pages = (size + PAGE_SIZE - 1) / PAGE_SIZE
        return pfa_alloc_n(pages)

    idx = bin_index(size)
    if free_list[idx] != NULL:
        block = free_list[idx]
        free_list[idx] = block->next
        return block

    // No free block: get a new page from PFA
    page = pfa_alloc()
    bin_size = 1 << (4 + idx)   // 16, 32, 64, ...
    num_blocks = PAGE_SIZE / bin_size

    // Slice the page into blocks and add to free list
    for i = 0 to num_blocks - 1:
        block = page + (i * bin_size)
        block->next = free_list[idx]
        free_list[idx] = block

    // Now pop one from the newly populated list
    return kmalloc(size)   // or pop directly
```

#### Free (`kfree`)

```
void kfree(void *ptr, size_t size):
    if size > MAX_BLOCK_SIZE:
        pages = (size + PAGE_SIZE - 1) / PAGE_SIZE
        pfa_free_n(ptr, pages)
        return

    idx = bin_index(size)
    block = (block_header_t *)ptr
    block->next = free_list[idx]
    free_list[idx] = block
```

**Note:** In a real OS, `kfree(ptr)` doesn't take a size. To do that, store metadata _before_ the returned pointer (a short header).

### 3. Power-of-Two Sizing

Using power-of-2 sizes (16, 32, 64, ...) has benefits:

- **Alignment:** Every block is naturally aligned to its size.
- **Determinism:** No fragmentation within a page (a 64-byte block always fits exactly 64 times in a page).
- **Simplicity:** No need for a merge/coalesce pass.

Tradeoff: **Internal fragmentation.** A 17-byte request uses a 32-byte block, wasting 15 bytes.

### 4. Metadata Header Approach (for size-tracking `kfree`)

```
typedef struct {
    size_t size;        // requested size (for sanity checks)
    uint32_t magic;     // e.g., 0xDEADBEEF (detect double-free or corruption)
} alloc_header_t;

void *kmalloc(size_t size):
    total = sizeof(alloc_header_t) + size
    // round total up to nearest bin size
    ptr = alloc_from_bin(total)
    header = (alloc_header_t *)ptr
    header->size = size
    header->magic = HEAP_MAGIC
    return ptr + sizeof(alloc_header_t)

void kfree(void *ptr):
    if ptr == NULL: return
    header = ptr - sizeof(alloc_header_t)
    assert(header->magic == HEAP_MAGIC)   // catch corruption
    header->magic = 0xBADDCAFE             // mark as freed
    return_to_bin(header, bin_for_size(header->size))
```

### 5. Buddy Allocator (Alternative)

Instead of fixed-size bins, you could use a **buddy system**:

- All allocations are rounded up to the next power of 2.
- A free block of size 2^k can be split into two "buddy" blocks of size 2^(k-1).
- When a block is freed, check if its buddy is also free; if so, merge back to 2^k.
- The Linux kernel's page allocator is a buddy allocator.

**Pros:** No internal fragmentation (well, up to rounding). Handles arbitrary sizes.
**Cons:** More complex bookkeeping. Slower alloc/free than segregated lists.

For a first kernel heap, **segregated freelists** are recommended.

---

## Step-by-Step Implementation Path

### Step 1: Decide on a memory region for the heap

- Pre-allocate a chunk of pages for the heap at boot, or grow on-demand.
- Example: Start with 8 pages (32 KiB) dedicated to the heap.

### Step 2: Implement the segregated freelist

- Define your bins (16, 32, 64, 128, 256, 512, 1024, 2048).
- Write `kmalloc()` and `kfree()` with header tracking.

### Step 3: Add safety checks

- Magic number in header to detect use-after-free and double-free.
- Poison freed memory with `0xDEAD` or `0xFEEEFEEE` in debug builds.

### Step 4: Implement page-return optimization

- When `kfree()` returns the last block in a page, that page can go back to the PFA.
- Track how many blocks in each page are currently allocated.
- Add a per-page reference count.

### Step 5: Test with kernel data structures

```
// Pseudocode
struct node *head = NULL;
for i = 0 to 100:
    n = kmalloc(sizeof(struct node));
    n->data = i;
    n->next = head;
    head = n;

while head != NULL:
    tmp = head;
    head = head->next;
    kfree(tmp);
```

---

## Key Questions to Answer Before Moving On

1. Why can't you just use `pfa_alloc()` for every small allocation (e.g., a 16-byte struct)?
2. What is **internal fragmentation**? How does power-of-2 binning make it predictable?
3. How would you detect a **double-free** bug using the magic number in the header?
4. If your heap runs out of free blocks in a bin, what should it do?
5. Why is `kfree(NULL)` a valid operation that should silently return?

---

## Recommended Resources

| Resource                                                        | URL                                                                                                       | Why Read It                                                                                                                                                   |
| --------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Linux Kernel Docs — Memory Management**                       | https://docs.kernel.org/mm/index.html                                                                     | Official coverage of the page allocator (buddy system), `vmalloc`, `kmalloc`, slab/SLUB/SLAB.                                                                 |
| **Linux Kernel Docs — Memory Allocation APIs**                  | https://docs.kernel.org/core-api/mm-api.html                                                              | Reference for `kmalloc`, `kfree`, `kzalloc`, `krealloc`, and kernel allocator API contracts and gfp flags.                                                    |
| **LWN.net — The SLUB Allocator / Slab Internals**               | https://lwn.net/Articles/229230/ (SLUB overview) https://lwn.net/Articles/250967/ (Slab design deep dive) | Classic kernel-internals articles explaining slab cache construction, object tracking, and fragmentation reduction.                                           |
| **Understanding the Linux Virtual Memory Manager (Mel Gorman)** | https://www.kernel.org/doc/gorman/html/understand/                                                        | Full book freely available online. Covers buddy allocator mechanics, page tables, and kernel memory layout—concepts directly transferable to a custom kernel. |
| **OSDev Wiki — Memory Allocation**                              | https://wiki.osdev.org/Memory_Allocation                                                                  | General overview of kernel allocators: simple freelists, slab allocators, buddy systems, and kmalloc API design.                                              |
| **The Art of Computer Programming, Vol 1 — Knuth**              | (Book)                                                                                                    | Chapter on dynamic storage allocation. Covers buddy systems, boundary tags, and coalescing. A classic.                                                        |
| **"Writing a Simple Kernel Heap" tutorials**                    | Search: "kernel heap allocator tutorial bare metal"                                                       | Blog posts showing simple freelist and slab implementations. Good for cross-checking your design.                                                             |
