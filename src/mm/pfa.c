#include "pfa.h"
#include <stddef.h>
#include <stdint.h>

pageframeT startframe   = 0;
uint64_t   npages       = 0;
uint64_t  *bitmap       = NULL;
uint64_t   nextFreeIdx  = 0;

void pfaInit(uint64_t base, uint64_t size, uint64_t reservedStart, uint64_t reservedEnd) {
    startframe = base;
    npages     = size / PAGE_SIZE;
    nextFreeIdx = 0;

    uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
    for (uint64_t i = 0; i < totalWords; i++) {
        bitmap[i] = 0;
    }

    uint64_t reservedPages = (reservedEnd - startframe + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < reservedPages; i++) {
        uint64_t wordIndex = i / BITS_PER_WORD;
        uint64_t bitOffset = i % BITS_PER_WORD;
        bitmap[wordIndex] |= (1ULL << bitOffset);
    }

    nextFreeIdx = reservedPages;
}

pageframeT kallocFrame(void) {
    uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
    uint64_t startWord  = nextFreeIdx / BITS_PER_WORD;

    for (uint64_t i = startWord; i < totalWords; i++) {
        if (bitmap[i] != ~0ULL) {
            uint64_t bit       = __builtin_ctzll(~bitmap[i]);
            uint64_t pageIndex = (i * BITS_PER_WORD) + bit;

            if (pageIndex >= npages) {
                break;
            }

            bitmap[i] |= (1ULL << bit);
            nextFreeIdx = pageIndex + 1;
            return startframe + (pageIndex * PAGE_SIZE);
        }
    }

    for (uint64_t i = 0; i < startWord; i++) {
        if (bitmap[i] != ~0ULL) {
            uint64_t bit       = __builtin_ctzll(~bitmap[i]);
            uint64_t pageIndex = (i * BITS_PER_WORD) + bit;

            bitmap[i] |= (1ULL << bit);
            nextFreeIdx = pageIndex + 1;
            return startframe + (pageIndex * PAGE_SIZE);
        }
    }

    return ERROR;
}

void kfreeFrame(pageframeT pa) {
    if (pa < startframe)
        return;

    uint64_t index = (pa - startframe) / PAGE_SIZE;

    if (index < npages) {
        uint64_t wordIndex = index / BITS_PER_WORD;
        uint64_t bitOffset = index % BITS_PER_WORD;

        bitmap[wordIndex] &= ~(1ULL << bitOffset);

        if (index < nextFreeIdx) {
            nextFreeIdx = index;
        }
    }
}