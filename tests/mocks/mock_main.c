/* ============================================================================
 * Mock main support for host-side testing.
 * Provides the endkernel linker symbol and PFA bitmap storage
 * that the real kernel gets from its linker script and boot RAM.
 * ============================================================================ */
#include "mm/pfa.h"
#include <stdint.h>
#include <string.h>

uint32_t endkernel = 0;

static uint64_t pfa_bitmap_storage[4096];

void mockPfaSetup(void) {
    startframe  = 0;
    npages      = 0;
    bitmap      = pfa_bitmap_storage;
    nextFreeIdx = 0;
    memset(pfa_bitmap_storage, 0, sizeof(pfa_bitmap_storage));
}