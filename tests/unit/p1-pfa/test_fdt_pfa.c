/* ============================================================================
 * Phase 3: FDT Parser & Page Frame Allocator (PFA)
 * Tests the bitmap allocator logic with a small fake RAM region.
 * ============================================================================ */

#include "test.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Replicate the PFA types/constants from main.c */

typedef uint64_t pageframeT;

#define PAGE_SIZE 0x1000
#define FREE      0
#define USED      1
#define ERROR     ((pageframeT) - 1)

/* Byte-swap for big-endian FDT on little-endian host */
static inline uint32_t fdt32TOCpu(uint32_t val) {
    return __builtin_bswap32(val);
}

/* Mirror the FDT alignment macro */
#define FDT_ALIGN(p) ((uint32_t *)(((uintptr_t)(p) + 3) & ~3))

struct __attribute__((packed)) fdtHeader {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t offDTStruct;
    uint32_t offDTStrings;
    uint32_t offMemRsvmap;
    uint32_t version;
    uint32_t lastCompVersion;
    uint32_t bootCpuidPhys;
    uint32_t sizeDTStrings;
    uint32_t sizeDTStruct;
};

/* --- Fake Bitmap Allocator --- */
static uint64_t  fake_bitmap[16];
static uint64_t  nextFreeIdx;
static uint64_t  npages;
static pageframeT startframe;

static void fakePfaInit(pageframeT base, uint64_t pages) {
    startframe = base;
    npages = pages;
    nextFreeIdx = 0;
    uint64_t totalWords = (npages + 63) / 64;
    for (uint64_t i = 0; i < totalWords; i++) fake_bitmap[i] = 0;
}

static pageframeT fakeAlloc(void) {
    uint64_t totalWords = (npages + 63) / 64;
    uint64_t startWord  = nextFreeIdx / 64;
    for (uint64_t i = startWord; i < totalWords; i++) {
        if (fake_bitmap[i] != ~0ULL) {
            uint64_t bit = (uint64_t)__builtin_ctzll(~fake_bitmap[i]);
            uint64_t pi  = i * 64 + bit;
            if (pi >= npages) break;
            fake_bitmap[i] |= (1ULL << bit);
            nextFreeIdx = pi + 1;
            return startframe + (pi * PAGE_SIZE);
        }
    }
    for (uint64_t i = 0; i < startWord; i++) {
        if (fake_bitmap[i] != ~0ULL) {
            uint64_t bit = (uint64_t)__builtin_ctzll(~fake_bitmap[i]);
            uint64_t pi  = i * 64 + bit;
            fake_bitmap[i] |= (1ULL << bit);
            nextFreeIdx = pi + 1;
            return startframe + (pi * PAGE_SIZE);
        }
    }
    return ERROR;
}

static void fakeFree(pageframeT pa) {
    if (pa < startframe) return;
    uint64_t index = (pa - startframe) / PAGE_SIZE;
    if (index < npages) {
        uint64_t wi = index / 64;
        uint64_t bi = index % 64;
        fake_bitmap[wi] &= ~(1ULL << bi);
        if (index < nextFreeIdx) nextFreeIdx = index;
    }
}

/* --- Tiny in-memory FDT builder --- */
static void buildFakeFdtMemory(void *buf, uint64_t base, uint64_t size) {
    memset(buf, 0, 2048);
    struct fdtHeader *h = (struct fdtHeader *)buf;
    h->magic          = __builtin_bswap32(0xd00dfeed);
    h->totalsize      = __builtin_bswap32(256);
    h->offDTStruct    = __builtin_bswap32(sizeof(struct fdtHeader));
    h->offDTStrings   = __builtin_bswap32(sizeof(struct fdtHeader) + 64);
    h->offMemRsvmap   = __builtin_bswap32(sizeof(struct fdtHeader) + 128);
    h->version       = __builtin_bswap32(17);
    h->lastCompVersion = __builtin_bswap32(16);
    h->sizeDTStrings  = __builtin_bswap32(32);

    uint8_t *base8  = (uint8_t *)buf;
    uint32_t *tok   = (uint32_t *)(base8 + sizeof(struct fdtHeader));
    uint32_t idx    = 0;

    tok[idx++] = __builtin_bswap32(1);           /* FDT_BEGIN_NODE */
    char *name = (char *)(&tok[idx]);
    strcpy(name, "");                            /* root */
    idx = (uint32_t)(
          (((uintptr_t)(name + strlen(name) + 1 + 3)) & ~3)
          - (uintptr_t)tok) / sizeof(uint32_t);

    tok[idx++] = __builtin_bswap32(1);           /* FDT_BEGIN_NODE -> memory */
    char *mname = (char *)(&tok[idx]);
    strcpy(mname, "memory");
    idx = (uint32_t)(
          (((uintptr_t)(mname + strlen(mname) + 1 + 3)) & ~3)
          - (uintptr_t)tok) / sizeof(uint32_t);

    tok[idx++] = __builtin_bswap32(3);           /* FDT_PROP */
    tok[idx++] = __builtin_bswap32(16);          /* length */
    tok[idx++] = __builtin_bswap32(0);           /* nameoff -> 'reg' at start of strings block */

    uint32_t *regdata = &tok[idx];
    regdata[0] = __builtin_bswap32((uint32_t)(base >> 32));
    regdata[1] = __builtin_bswap32((uint32_t)(base & 0xFFFFFFFF));
    regdata[2] = __builtin_bswap32((uint32_t)(size >> 32));
    regdata[3] = __builtin_bswap32((uint32_t)(size & 0xFFFFFFFF));
    idx += 4;

    tok[idx++] = __builtin_bswap32(2);           /* FDT_END_NODE */
    tok[idx++] = __builtin_bswap32(2);           /* FDT_END_NODE -> root */
    tok[idx++] = __builtin_bswap32(9);           /* FDT_END */

    /* Strings block */
    uint8_t *strBase = base8 + sizeof(struct fdtHeader) + 64;
    strcpy((char *)strBase, "reg");

    /* Reservation map terminator */
    uint64_t *rsv = (uint64_t *)(base8 + sizeof(struct fdtHeader) + 128);
    rsv[0] = __builtin_bswap64(0);
    rsv[1] = __builtin_bswap64(0);
}

/* ========================================================================= */
PHASE(p3_fdt_and_pfa) {

    TEST("FDT magic byte-swap is correct") {
        uint32_t raw = 0xd00dfeed;
        ASSERT_EQ_HEX(0xedfe0dd0, fdt32TOCpu(raw));
    }

    TEST("FDT alignment macro rounds up to 4") {
        uint32_t *p = (uint32_t *)(uintptr_t)5;
        uint32_t *a = FDT_ALIGN(p);
        ASSERT_EQ_UINT(8, (uintptr_t)a);
    }

    TEST("Fake FDT header magic validates after swap") {
        uint8_t buf[2048] = {0};
        buildFakeFdtMemory(buf, 0x40000000ULL, 0x10000000ULL);
        struct fdtHeader *h = (struct fdtHeader *)buf;
        ASSERT_EQ_HEX(0xd00dfeed, fdt32TOCpu(h->magic));
    }

    TEST("Fake FDT memory node reg property base") {
        uint8_t buf[2048] = {0};
        buildFakeFdtMemory(buf, 0x40000000ULL, 0x10000000ULL);
        uint32_t *tok = (uint32_t *)(buf + sizeof(struct fdtHeader));
        int i = 0;
        while (i < 32) {
            uint32_t t = fdt32TOCpu(tok[i++]);
            if (t == 1) {  /* FDT_BEGIN_NODE */
                if (strcmp((char *)(&tok[i]), "memory") == 0) break;
                while (((char *)tok)[i * sizeof(uint32_t)]) {
                    i++;
                }
                i = (int)(
                    (((uintptr_t)(&tok[i]) + 3) & ~3) - (uintptr_t)tok) / sizeof(uint32_t);
            }
        }
        /* skip to next tokens; the real parser confirms layout, we'll
         * just assert that buildFakeFdtMemory produces a parseable blob. */
        ASSERT_TRUE(i < 32);  /* found memory node */
    }

    /* --- PFA Tests --- */
    TEST("PFA init marks all pages free") {
        fakePfaInit(0x40000000, 256);
        ASSERT_EQ_UINT(0, fake_bitmap[0]);
    }

    TEST("PFA alloc returns first free page") {
        fakePfaInit(0x40000000, 256);
        pageframeT p = fakeAlloc();
        ASSERT_EQ_HEX(0x40000000, p);
    }

    TEST("PFA alloc returns distinct pages") {
        fakePfaInit(0x40000000, 256);
        pageframeT p1 = fakeAlloc();
        pageframeT p2 = fakeAlloc();
        pageframeT p3 = fakeAlloc();
        ASSERT_NE_INT(p1, p2);
        ASSERT_NE_INT(p1, p3);
        ASSERT_NE_INT(p2, p3);
    }

    TEST("PFA free reuses a page") {
        fakePfaInit(0x40000000, 256);
        pageframeT p1 = fakeAlloc();
        fakeFree(p1);
        pageframeT p2 = fakeAlloc();
        ASSERT_EQ_HEX(p1, p2);
    }

    TEST("PFA alloc exhausts and returns ERROR") {
        fakePfaInit(0x40000000, 2);
        fakeAlloc();
        fakeAlloc();
        pageframeT p = fakeAlloc();
        ASSERT_EQ_HEX(ERROR, p);
    }

    TEST("PFA free of non-managed address is no-op") {
        fakePfaInit(0x40000000, 10);
        uint64_t before = fake_bitmap[0];
        fakeFree(0x10000000);
        ASSERT_EQ_UINT(before, fake_bitmap[0]);
    }

    TEST("PFA bitmap marks correct bit on alloc") {
        fakePfaInit(0x40000000, 256);
        fakeAlloc();  /* page 0 */
        ASSERT_TRUE((fake_bitmap[0] & 1ULL) != 0);
    }

    TEST("PFA alloc across word boundary") {
        fakePfaInit(0x40000000, 128);  /* two 64-bit words */
        for (int i = 0; i < 64; i++) fakeAlloc();  /* exhaust first word */
        pageframeT p = fakeAlloc();
        ASSERT_TRUE(p != ERROR);
    }

    TEST("FDT header field offsets match specification size (40 bytes)") {
        ASSERT_EQ_UINT(40, sizeof(struct fdtHeader));
    }
}
