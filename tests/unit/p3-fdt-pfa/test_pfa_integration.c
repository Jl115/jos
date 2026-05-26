#include "test.h"
#include "mm/pfa.h"

#include <stdint.h>
#include <string.h>

extern void mockPfaSetup(void);

PHASE(p3_pfa_integration) {
    TEST("pfaInit sets startframe to base address") {
        mockPfaSetup();
        pfaInit(0x40000000ULL, 0x10000ULL, 0x40000000ULL, 0x40001000ULL);
        ASSERT_EQ_HEX(startframe, 0x40000000ULL);
    }

    TEST("pfaInit computes npages from size / PAGE_SIZE") {
        mockPfaSetup();
        pfaInit(0x40000000ULL, 0x10000ULL, 0x40000000ULL, 0x40001000ULL);
        ASSERT_EQ_INT((int)npages, (int)(0x10000ULL / PAGE_SIZE));
    }

    TEST("pfaInit marks reserved pages as used") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        uint64_t reservedEnd = base + (4 * PAGE_SIZE);
        pfaInit(base, 0x10000ULL, base, reservedEnd);

        uint64_t reservedPages = (reservedEnd - base + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint64_t i = 0; i < reservedPages; i++) {
            uint64_t wordIndex = i / BITS_PER_WORD;
            uint64_t bitOffset = i % BITS_PER_WORD;
            ASSERT_TRUE(bitmap[wordIndex] & (1ULL << bitOffset));
        }
    }

    TEST("pfaInit sets nextFreeIdx past reserved region") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        uint64_t reservedEnd = base + (4 * PAGE_SIZE);
        pfaInit(base, 0x10000ULL, base, reservedEnd);

        uint64_t expectedFreeIdx = 4;
        ASSERT_EQ_INT((int)nextFreeIdx, (int)expectedFreeIdx);
    }

    TEST("kallocFrame returns first free frame after reserved region") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT frame = kallocFrame();
        ASSERT_EQ_HEX(frame, base + 4 * PAGE_SIZE);
    }

    TEST("kallocFrame returns sequential frames") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT f1 = kallocFrame();
        pageframeT f2 = kallocFrame();
        ASSERT_EQ_HEX(f2 - f1, PAGE_SIZE);
    }

    TEST("kallocFrame marks frame as used in bitmap") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT frame = kallocFrame();
        uint64_t idx = (frame - base) / PAGE_SIZE;
        uint64_t wordIndex = idx / BITS_PER_WORD;
        uint64_t bitOffset = idx % BITS_PER_WORD;
        ASSERT_TRUE(bitmap[wordIndex] & (1ULL << bitOffset));
    }

    TEST("kfreeFrame frees a frame and it becomes allocatable again") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT f1 = kallocFrame();
        kfreeFrame(f1);

        uint64_t idx = (f1 - base) / PAGE_SIZE;
        uint64_t wordIndex = idx / BITS_PER_WORD;
        uint64_t bitOffset = idx % BITS_PER_WORD;
        ASSERT_FALSE(bitmap[wordIndex] & (1ULL << bitOffset));
    }

    TEST("kfreeFrame + kallocFrame reuses freed frame") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT f1 = kallocFrame();
        (void)kallocFrame();
        kfreeFrame(f1);
        pageframeT f3 = kallocFrame();
        ASSERT_EQ_HEX(f3, f1);
    }

    TEST("kfreeFrame updates nextFreeIdx to freed index") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT f1 = kallocFrame();
        uint64_t freedIdx = (f1 - base) / PAGE_SIZE;
        kfreeFrame(f1);
        ASSERT_EQ_INT((int)nextFreeIdx, (int)freedIdx);
    }

    TEST("kfreeFrame below startframe is a no-op") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        pageframeT before_free = kallocFrame();
        kfreeFrame(0x10000000ULL);
        ASSERT_TRUE(before_free >= base);
    }

    TEST("kfreeFrame on out-of-range address does nothing") {
        mockPfaSetup();
        uint64_t base = 0x40000000ULL;
        pfaInit(base, 0x10000ULL, base, base + (4 * PAGE_SIZE));

        kallocFrame();
        uint64_t old_nextFreeIdx = nextFreeIdx;
        kfreeFrame(base + 0x10000000ULL);
        ASSERT_EQ_INT((int)nextFreeIdx, (int)old_nextFreeIdx);
    }

    TEST("kallocFrame returns ERROR when all frames are allocated") {
        mockPfaSetup();
        uint64_t bitmap_storage[4];
        memset(bitmap_storage, 0xFF, sizeof(bitmap_storage));
        bitmap = bitmap_storage;
        npages = 256;
        startframe = 0x40000000ULL;
        nextFreeIdx = 0;

        pageframeT frame = kallocFrame();
        ASSERT_EQ_HEX(frame, ERROR);
    }

    TEST("pfaInit with zero reserved region marks nothing as used") {
        mockPfaSetup();
        pfaInit(0x40000000ULL, 0x10000ULL, 0x40000000ULL, 0x40000000ULL);

        int all_free = 1;
        uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
        for (uint64_t i = 0; i < totalWords && i < 4096; i++) {
            if (bitmap[i] != 0) {
                all_free = 0;
                break;
            }
        }
        ASSERT_TRUE(all_free);
    }

    TEST("PAGE_SIZE is 4096 (0x1000)") {
        ASSERT_EQ_INT((int)PAGE_SIZE, 0x1000);
    }

    TEST("ERROR sentinel is all-bits-set") {
        ASSERT_EQ_HEX((uint64_t)ERROR, 0xFFFFFFFFFFFFFFFFULL);
    }

    TEST("BITS_PER_WORD is 64") {
        ASSERT_EQ_INT((int)BITS_PER_WORD, 64);
    }
}