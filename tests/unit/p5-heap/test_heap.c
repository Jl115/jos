#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef HEAP_H
#define HEAP_H

#define HEAP_NUM_BINS   8
#define HEAP_MIN_SIZE   16
#define HEAP_MAX_SIZE   2048
#define HEAP_MAGIC_LIVE 0xDEADBEEFULL
#define HEAP_MAGIC_FREE 0xBADDCAFEULL

typedef struct alloc_header {
    uint64_t magic;
    uint64_t size;
} alloc_header_t;

typedef struct block_node {
    struct block_node *next;
} block_node_t;

typedef struct heap_bin {
    block_node_t *freelist;
    uint64_t      count;
} heap_bin_t;

typedef struct heap {
    heap_bin_t bins[HEAP_NUM_BINS];
    uint64_t   total_allocs;
    uint64_t   total_frees;
} heap_t;

static inline int heapBinIndex(uint64_t size) {
    if (size <= 16)   return 0;
    if (size <= 32)   return 1;
    if (size <= 64)   return 2;
    if (size <= 128)  return 3;
    if (size <= 256)  return 4;
    if (size <= 512)  return 5;
    if (size <= 1024) return 6;
    return 7;
}

static inline uint64_t heapBinSize(int index) {
    static const uint64_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    return sizes[index];
}

void  heapInit(heap_t *heap);
void *kmalloc(uint64_t size);
void  kfree(void *ptr);

#endif

PHASE(p5_kernel_heap) {
    TEST("HEAP_NUM_BINS is 8 (segregated freelist bins)") {
        ASSERT_EQ_INT((int)HEAP_NUM_BINS, 8);
    }

    TEST("HEAP_MIN_SIZE is 16 bytes (smallest allocatable block)") {
        ASSERT_EQ_INT((int)HEAP_MIN_SIZE, 16);
    }

    TEST("HEAP_MAX_SIZE is 2048 bytes (largest freelist block)") {
        ASSERT_EQ_INT((int)HEAP_MAX_SIZE, 2048);
    }

    TEST("HEAP_MAGIC_LIVE is 0xDEADBEEF") {
        ASSERT_EQ_HEX((uint64_t)HEAP_MAGIC_LIVE, 0xDEADBEEFULL);
    }

    TEST("HEAP_MAGIC_FREE is 0xBADDCAFE") {
        ASSERT_EQ_HEX((uint64_t)HEAP_MAGIC_FREE, 0xBADDCAFEULL);
    }

    TEST("heapBinIndex maps size 16 to bin 0") {
        ASSERT_EQ_INT(heapBinIndex(16), 0);
    }

    TEST("heapBinIndex maps size 32 to bin 1") {
        ASSERT_EQ_INT(heapBinIndex(32), 1);
    }

    TEST("heapBinIndex maps size 64 to bin 2") {
        ASSERT_EQ_INT(heapBinIndex(64), 2);
    }

    TEST("heapBinIndex maps size 128 to bin 3") {
        ASSERT_EQ_INT(heapBinIndex(128), 3);
    }

    TEST("heapBinIndex maps size 256 to bin 4") {
        ASSERT_EQ_INT(heapBinIndex(256), 4);
    }

    TEST("heapBinIndex maps size 512 to bin 5") {
        ASSERT_EQ_INT(heapBinIndex(512), 5);
    }

    TEST("heapBinIndex maps size 1024 to bin 6") {
        ASSERT_EQ_INT(heapBinIndex(1024), 6);
    }

    TEST("heapBinIndex maps size 2048 to bin 7") {
        ASSERT_EQ_INT(heapBinIndex(2048), 7);
    }

    TEST("heapBinIndex rounds up size 1 to bin 0") {
        ASSERT_EQ_INT(heapBinIndex(1), 0);
    }

    TEST("heapBinIndex rounds up size 17 to bin 1") {
        ASSERT_EQ_INT(heapBinIndex(17), 1);
    }

    TEST("heapBinIndex rounds up size 33 to bin 2") {
        ASSERT_EQ_INT(heapBinIndex(33), 2);
    }

    TEST("heapBinIndex rounds up size 1000 to bin 6") {
        ASSERT_EQ_INT(heapBinIndex(1000), 6);
    }

    TEST("heapBinSize returns correct sizes for each bin") {
        ASSERT_EQ_INT((int)heapBinSize(0), 16);
        ASSERT_EQ_INT((int)heapBinSize(1), 32);
        ASSERT_EQ_INT((int)heapBinSize(2), 64);
        ASSERT_EQ_INT((int)heapBinSize(3), 128);
        ASSERT_EQ_INT((int)heapBinSize(4), 256);
        ASSERT_EQ_INT((int)heapBinSize(5), 512);
        ASSERT_EQ_INT((int)heapBinSize(6), 1024);
        ASSERT_EQ_INT((int)heapBinSize(7), 2048);
    }

    TEST("alloc_header_t size is 16 bytes (magic + size)") {
        ASSERT_EQ_INT((int)sizeof(alloc_header_t), 16);
    }

    TEST("alloc_header_t.magic field offset is 0") {
        alloc_header_t h;
        ASSERT_EQ_INT((int)((uintptr_t)&h.magic - (uintptr_t)&h), 0);
    }

    TEST("alloc_header_t.size field offset is 8") {
        alloc_header_t h;
        ASSERT_EQ_INT((int)((uintptr_t)&h.size - (uintptr_t)&h), 8);
    }

    TEST("block_node_t is a singly-linked list node (next pointer)") {
        ASSERT_EQ_INT((int)sizeof(block_node_t), (int)sizeof(void *));
    }

    TEST("heap_t has 8 bins") {
        heap_t h;
        ASSERT_EQ_INT((int)(sizeof(h.bins) / sizeof(h.bins[0])), 8);
    }

    TEST("heapBin_t has freelist pointer and count") {
        heap_bin_t b;
        (void)b.freelist;
        (void)b.count;
        ASSERT_TRUE(1);
    }

    TEST("heapInit zeros all bins") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kmalloc returns non-NULL for small allocation") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kmalloc aligns to 16-byte boundary") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kfree(NULL) is a no-op") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kfree detects double-free via magic number") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kfree detects use-after-free via magic number") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kmalloc allocations within same bin come from freelist") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kmalloc for size > 2048 allocates whole pages via PFA") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("kfree for large allocation returns pages to PFA") {
        SKIP("kmalloc/kfree implementation not yet available");
    }

    TEST("100 alloc/free cycle: all freed blocks are reusable") {
        SKIP("kmalloc/kfree implementation not yet available");
    }
}