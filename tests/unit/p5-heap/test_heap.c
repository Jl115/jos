/* ============================================================================
 * Phase 5: Dynamic Memory (Kernel Heap)
 * Tests a miniature segregated-freelist allocator on the host.
 * ============================================================================ */

#include "test.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define HEAP_MAGIC 0xDEADBEEF
#define MIN_BIN    4    /* 16 bytes */
#define NUM_BINS   8    /* sizes: 16, 32, 64, 128, 256, 512, 1024, 2048 */
#define SMALL_SLAB 65536 /* max heap backed by a static buffer */

static uint8_t g_heap_slab[SMALL_SLAB];
static int     g_heap_pos = 0;

/* --- Minimal heap state --- */
typedef struct block_header {
    struct block_header *next;
} block_header_t;

static block_header_t *g_freelist[NUM_BINS] = {0};

static size_t bin_index(size_t size) {
    if (size <= 16) return 0;
    for (int i = 1; i < NUM_BINS; i++) {
        size_t cutoff = 16 << i;
        if (size <= cutoff) return i;
    }
    return NUM_BINS - 1;
}

static void *heapAlloc(size_t size) {
    size_t bin = bin_index(size);
    if (g_freelist[bin]) {
        block_header_t *b = g_freelist[bin];
        g_freelist[bin] = b->next;
        return (void *)((uintptr_t)b + sizeof(size_t) + sizeof(uint32_t));
    }

    size_t pageSize = 16 << bin;
    size_t totalSize = sizeof(size_t) + sizeof(uint32_t) + pageSize;
    if (g_heap_pos + (int)totalSize > SMALL_SLAB) return NULL;

    uint8_t *base = g_heap_slab + g_heap_pos;
    g_heap_pos += (int)totalSize;

    size_t *sz = (size_t *)base;
    uint32_t *mg = (uint32_t *)(base + sizeof(size_t));
    *sz = size;
    *mg = HEAP_MAGIC;
    return base + sizeof(size_t) + sizeof(uint32_t);
}

static void heapFree(void *ptr) {
    if (!ptr) return;
    void *base = (void *)((uintptr_t)ptr - sizeof(size_t) - sizeof(uint32_t));
    size_t *sz = (size_t *)base;
    uint32_t *mg = (uint32_t *)((uintptr_t)base + sizeof(size_t));
    *mg = 0xBADDCAFE;

    size_t bi = bin_index(*sz);
    block_header_t *h = (block_header_t *)base;
    h->next = g_freelist[bi];
    g_freelist[bi] = h;
}

static void heapReset(void) {
    g_heap_pos = 0;
    memset(g_heap_slab, 0xAB, SMALL_SLAB); /* poison */
    memset(g_freelist, 0, sizeof(g_freelist));
}

/* ========================================================================= */
PHASE(p5_kernel_heap) {

    TEST("Bin index for  8 bytes rounds up to 0 (16-byte bin)") {
        ASSERT_EQ_UINT(bin_index(8), 0);
    }

    TEST("Bin index for 16 bytes stays in bin 0") {
        ASSERT_EQ_UINT(bin_index(16), 0);
    }

    TEST("Bin index for 17 bytes goes to bin 1 (32)") {
        ASSERT_EQ_UINT(bin_index(17), 1);
    }

    TEST("Bin index for 2048 bytes lands in final bin") {
        ASSERT_EQ_UINT(bin_index(2048), NUM_BINS - 1);
    }

    TEST("kmalloc returns NULL when out of memory") {
        heapReset();
        while (heapAlloc(1)) {}   /* exhaust slab with smallest bin */
        ASSERT_PTR_NULL(heapAlloc(1));
    }

    TEST("kmalloc / kfree round-trip without crash") {
        heapReset();
        void *p = heapAlloc(64);
        ASSERT_PTR_NOT_NULL(p);
        heapFree(p);
    }

    TEST("Magic number is visible before the returned pointer") {
        heapReset();
        void *p = heapAlloc(64);
        uint32_t *mg = (uint32_t *)((uintptr_t)p - sizeof(uint32_t));
        ASSERT_EQ_UINT(HEAP_MAGIC, *mg);
    }

    TEST("Magic number changes to 0xBADDCAFE after free") {
        heapReset();
        void *p = heapAlloc(64);
        heapFree(p);
        uint32_t *mg = (uint32_t *)((uintptr_t)p - sizeof(uint32_t));
        ASSERT_EQ_UINT(0xBADDCAFE, *mg);
    }

    TEST("Reuse freed block from same bin") {
        heapReset();
        void *a = heapAlloc(64);
        heapFree(a);
        void *b = heapAlloc(64);
        ASSERT_EQ_HEX((uint64_t)a, (uint64_t)b);
    }

    TEST("Multiple allocations return distinct regions") {
        heapReset();
        void *p[5];
        for (int i = 0; i < 5; i++) {
            p[i] = heapAlloc(100);
            ASSERT_PTR_NOT_NULL(p[i]);
        }
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                ASSERT_NE_INT((uint64_t)p[i], (uint64_t)p[j]);
            }
        }
    }

    TEST("kfree(NULL) is safe") {
        heapReset();
        heapFree(NULL);
        ASSERT_EQ_INT(1, 1); /* no crash = pass */
    }

    TEST("Block state survives a memcmp after alloc") {
        heapReset();
        void *p = heapAlloc(32);
        memset(p, 0xCD, 32);
        uint8_t ref[32]; memset(ref, 0xCD, 32);
        ASSERT_MEM_EQ(p, ref, 32);
    }

    TEST("Object fits inside bin page size") {
        heapReset();
        void *p = heapAlloc(40);
        size_t *sz = (size_t *)((uintptr_t)p - sizeof(size_t) - sizeof(uint32_t));
        ASSERT_TRUE(*sz <= 64);
    }
}
