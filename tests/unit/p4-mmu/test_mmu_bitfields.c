/* ============================================================================
 * Phase 4: Virtual Memory (MMU & Paging)
 * Tests AArch64 translation-table descriptor construction logic.
 * No real MMU — this is pure bit-field arithmetic validation.
 * ============================================================================ */

#include "test.h"
#include <stdint.h>

/* AArch64 page descriptor helpers (mirroring the kernel's future logic) */
#define PAGE_SIZE (1 << 12)
#define DESC_VALID_TABLE   0x3ULL  /* bits [1:0] = 01 (block) or 11 (table/page) */
#define DESC_VALID_PAGE    0x3ULL
#define DESC_AF            (1ULL << 10)
#define DESC_AP_KERN_RW    (0b00ULL << 6)
#define DESC_AP_KERN_RO    (0b10ULL << 6)
#define DESC_AP_USER_RW    (0b01ULL << 6)
#define DESC_SH_INNER      (0b11ULL << 8)
#define DESC_ATTRIDX_NORMAL (0x0ULL << 2)
#define DESC_ATTRIDX_DEVICE (0x1ULL << 2)
#define DESC_XN            (1ULL << 54)

/* Virtual address bit extraction for 4 KiB granule, 48-bit VA */
#define VA_L0_IDX(va) (((va) >> 39) & 0x1FF)
#define VA_L1_IDX(va) (((va) >> 30) & 0x1FF)
#define VA_L2_IDX(va) (((va) >> 21) & 0x1FF)
#define VA_L3_IDX(va) (((va) >> 12) & 0x1FF)

static inline uint64_t makePageDescriptor(uint64_t pa, uint64_t attr) {
    return (pa & 0xFFFFFFFFF000ULL) | attr | DESC_VALID_PAGE | DESC_AF;
}

static inline uint64_t makeTableDescriptor(uint64_t pa) {
    return (pa & 0xFFFFFFFFF000ULL) | DESC_VALID_TABLE;
}

static inline uint64_t makeBlockDescriptor(uint64_t pa, uint64_t attr) {
    return (pa & 0xFFFFC0000000ULL) | attr | 0x1ULL | DESC_AF; /* block = bit[1:0]=01 */
}

/* ========================================================================= */
PHASE(p4_mmu_and_paging) {

    TEST("VA_L0_IDX extracts top 9 bits") {
        ASSERT_EQ_UINT(VA_L0_IDX(0xFFFFFF8000000000ULL), 0x1FF);
        ASSERT_EQ_UINT(VA_L0_IDX(0x0000000000000000ULL), 0);
        ASSERT_EQ_UINT(VA_L0_IDX(0x0000008000000000ULL), 1);
    }

    TEST("VA_L3_IDX extracts bottom 9 bits before page offset") {
        ASSERT_EQ_UINT(VA_L3_IDX(0x1000), 1);
        ASSERT_EQ_UINT(VA_L3_IDX(0x7FF000), 0x1FF);
        ASSERT_EQ_UINT(VA_L3_IDX(0x0), 0);
    }

    TEST("Page descriptor valid bits [1:0] are 0b11") {
        uint64_t desc = makePageDescriptor(0x40000000, DESC_ATTRIDX_NORMAL);
        ASSERT_EQ_UINT(desc & 0x3ULL, 0x3ULL);
    }

    TEST("Page descriptor output address aligned to 4 KiB") {
        uint64_t desc = makePageDescriptor(0x40001234, DESC_ATTRIDX_NORMAL);
        ASSERT_EQ_HEX(0x40001000, desc & 0xFFFFFFFFF000ULL);
    }

    TEST("Page descriptor includes Access Flag") {
        uint64_t desc = makePageDescriptor(0x40000000, DESC_ATTRIDX_NORMAL);
        ASSERT_TRUE((desc & DESC_AF) != 0);
    }

    TEST("Table descriptor overlaps bottom 12 bits with flags") {
        uint64_t desc = makeTableDescriptor(0x40001000);
        ASSERT_TRUE((desc & 0xFFF) == 0x003);
    }

    TEST("Device memory attribute index matches ATTR1 (0x1 << 2)" ) {
        uint64_t desc = makePageDescriptor(0x40000000, DESC_ATTRIDX_DEVICE);
        ASSERT_TRUE(((desc >> 2) & 0x7) == 1);
    }

    TEST("Normal memory attribute index matches ATTR0 (0x0 << 2)" ) {
        uint64_t desc = makePageDescriptor(0x40000000, DESC_ATTRIDX_NORMAL);
        ASSERT_TRUE(((desc >> 2) & 0x7) == 0);
    }

    TEST("Kernel R/W page has AP bits 0b00") {
        uint64_t desc = makePageDescriptor(0x40000000,
            DESC_ATTRIDX_NORMAL | DESC_AP_KERN_RW);
        ASSERT_EQ_UINT((desc >> 6) & 0x3, 0);
    }

    TEST("User R/W page has AP bits 0b01") {
        uint64_t desc = makePageDescriptor(0x40000000,
            DESC_ATTRIDX_NORMAL | DESC_AP_USER_RW);
        ASSERT_EQ_UINT((desc >> 6) & 0x3, 1);
    }

    TEST("Execute-never bit is set at descriptor bit 54") {
        uint64_t desc = makePageDescriptor(0x40000000,
            DESC_ATTRIDX_NORMAL | DESC_AP_KERN_RW | DESC_XN);
        ASSERT_TRUE((desc & (1ULL << 54)) != 0);
    }

    TEST("Block descriptor maps a 1 GiB region with bits[1:0]=01") {
        uint64_t desc = makeBlockDescriptor(0x40000000,
            DESC_ATTRIDX_NORMAL | DESC_AP_KERN_RW);
        ASSERT_EQ_UINT(desc & 0x3ULL, 0x1ULL);
    }

    TEST("L0 table holds 512 8-byte entries = 4096 bytes") {
        ASSERT_EQ_UINT(512 * 8, 4096);
    }

    TEST("L3 page count for 1 GiB region") {
        /* 1 GiB / 4 KiB = 262,144 pages */
        ASSERT_EQ_UINT((1ULL << 30) / PAGE_SIZE, 262144);
    }

    TEST("Identity map: PA == VA after descriptor construction") {
        uint64_t pa = 0x40000000;
        uint64_t desc = makePageDescriptor(pa, DESC_ATTRIDX_NORMAL);
        uint64_t extracted_pa = desc & 0xFFFFFFFFF000ULL;
        ASSERT_EQ_HEX(pa, extracted_pa);
    }
}
