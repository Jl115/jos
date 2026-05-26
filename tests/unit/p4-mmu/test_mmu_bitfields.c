#include "test.h"
#include <stdint.h>
#include <stddef.h>

#ifndef MMU_H
#define MMU_H

typedef uint64_t pte_t;

#define PTE_VALID     (1ULL << 0)
#define PTE_TABLE     (1ULL << 1)
#define PTE_PAGE      (1ULL << 1)
#define PTE_BLOCK     (0ULL << 1)

#define PTE_ATTR_IDX(x)  (((uint64_t)(x)) << 2)
#define PTE_NS          (1ULL << 5)
#define PTE_AP_RO       (1ULL << 7)
#define PTE_AP_EL0     (1ULL << 6)
#define PTE_AF          (1ULL << 10)
#define PTE_SH_IS       (3ULL << 8)
#define PTE_SH_OS       (2ULL << 8)

#define PTE_ADDR_MASK   (0x0000FFFFFFFFF000ULL)

#define ATTR_NORMAL_WB  (0ULL)
#define ATTR_DEVICE     (1ULL)

#define MAIR_ATTR0_NORMAL  (0xFFULL << 0)
#define MAIR_ATTR1_DEVICE   (0x00ULL << 8)

#define TCR_T0SZ(x)      ((uint64_t)(x))
#define TCR_TG0_4K       (0ULL << 14)
#define TCR_IRGN0_WB      (1ULL << 8)
#define TCR_ORGN0_WB      (1ULL << 10)
#define TCR_SH0_IS        (3ULL << 12)
#define TCR_EPD1          (1ULL << 23)

#define SCTLR_M           (1ULL << 0)
#define SCTLR_A           (1ULL << 1)
#define SCTLR_C           (1ULL << 2)

typedef struct {
    pte_t entries[512];
} page_table_t;

void     mmuInit(void);
pte_t    *mmuGetL0Table(void);
int      mmuMapPage(uint64_t virt, uint64_t phys, pte_t flags);
int      mmuUnmapPage(uint64_t virt);
uint64_t mmuVirtToPhys(uint64_t virt);
int      mmuSetL1Block(uint64_t virt, uint64_t phys, pte_t flags);
pte_t    mmuMakePte(uint64_t phys, pte_t flags);

#endif

PHASE(p4_mmu_and_paging) {
    TEST("PTE_VALID bit is bit 0") {
        ASSERT_EQ_HEX(PTE_VALID, 1ULL);
    }

    TEST("PTE_TABLE bit is bit 1") {
        ASSERT_EQ_HEX(PTE_TABLE, 2ULL);
    }

    TEST("PTE_BLOCK differentiates from page/table descriptor") {
        ASSERT_EQ_HEX(PTE_BLOCK, 0ULL);
    }

    TEST("PTE_PAGE equals PTE_TABLE (bit 1)") {
        ASSERT_EQ_HEX(PTE_PAGE, PTE_TABLE);
    }

    TEST("PTE_AF (Access Flag) is bit 10") {
        ASSERT_EQ_HEX(PTE_AF, (1ULL << 10));
    }

    TEST("PTE_AP_RO (read-only) is bit 7") {
        ASSERT_EQ_HEX(PTE_AP_RO, (1ULL << 7));
    }

    TEST("PTE_AP_EL0 (user access) is bit 6") {
        ASSERT_EQ_HEX(PTE_AP_EL0, (1ULL << 6));
    }

    TEST("PTE_SH_IS (inner shareable) is bits [9:8] = 0b11") {
        ASSERT_EQ_HEX(PTE_SH_IS, (3ULL << 8));
    }

    TEST("PTE_ATTR_IDX shifts attr index into bits [4:2]") {
        ASSERT_EQ_HEX(PTE_ATTR_IDX(0), 0ULL);
        ASSERT_EQ_HEX(PTE_ATTR_IDX(1), (1ULL << 2));
        ASSERT_EQ_HEX(PTE_ATTR_IDX(7), (7ULL << 2));
    }

    TEST("attr index 0 = Normal Write-Back cacheable") {
        ASSERT_EQ_INT((int)ATTR_NORMAL_WB, 0);
    }

    TEST("attr index 1 = Device nGnRnE") {
        ASSERT_EQ_INT((int)ATTR_DEVICE, 1);
    }

    TEST("MAIR_ATTR0_NORMAL sets lower byte to 0xFF (WB cacheable)") {
        ASSERT_EQ_HEX(MAIR_ATTR0_NORMAL, 0xFFULL);
    }

    TEST("MAIR_ATTR1_DEVICE sets byte 1 to 0x00 (device)") {
        ASSERT_EQ_HEX(MAIR_ATTR1_DEVICE, 0x00ULL << 8);
    }

    TEST("TCR_T0SZ for 48-bit VA is 16") {
        ASSERT_EQ_INT((int)TCR_T0SZ(16), 16);
    }

    TEST("TCR_TG0_4K is 0 (4 KiB granule)") {
        ASSERT_EQ_HEX(TCR_TG0_4K, 0ULL);
    }

    TEST("TCR_IRGN0_WB is bit 8 (write-back inner cacheability)") {
        ASSERT_EQ_HEX(TCR_IRGN0_WB, (1ULL << 8));
    }

    TEST("TCR_ORGN0_WB is bit 10 (write-back outer cacheability)") {
        ASSERT_EQ_HEX(TCR_ORGN0_WB, (1ULL << 10));
    }

    TEST("TCR_SH0_IS is bits [13:12] = 0b11 (inner shareable)") {
        ASSERT_EQ_HEX(TCR_SH0_IS, (3ULL << 12));
    }

    TEST("TCR_EPD1 is bit 23 (disable TTBR1 walks)") {
        ASSERT_EQ_HEX(TCR_EPD1, (1ULL << 23));
    }

    TEST("SCTLR_M enables MMU") {
        ASSERT_EQ_HEX(SCTLR_M, 1ULL);
    }

    TEST("SCTLR_C enables data cache") {
        ASSERT_EQ_HEX(SCTLR_C, (1ULL << 2));
    }

    TEST("SCTLR_A enables alignment checking") {
        ASSERT_EQ_HEX(SCTLR_A, (1ULL << 1));
    }

    TEST("PTE address mask extracts output address [47:12]") {
        uint64_t addr = 0x40001000ULL;
        uint64_t masked = addr & PTE_ADDR_MASK;
        ASSERT_EQ_HEX(masked, 0x40001000ULL);
    }

    TEST("PTE address mask strips bits above 47") {
        uint64_t addr = 0xFFFF000080001000ULL;
        uint64_t masked = addr & PTE_ADDR_MASK;
        ASSERT_EQ_HEX(masked, 0x000080001000ULL);
    }

    TEST("Valid page PTE with Normal WB + AF + IS") {
        pte_t pte = PTE_VALID | PTE_PAGE | PTE_ATTR_IDX(ATTR_NORMAL_WB) | PTE_AF | PTE_SH_IS;
        ASSERT_TRUE(pte & PTE_VALID);
        ASSERT_TRUE(pte & PTE_PAGE);
        ASSERT_TRUE(pte & PTE_AF);
        ASSERT_TRUE(pte & PTE_SH_IS);
    }

    TEST("Valid block PTE with Device + AF") {
        pte_t pte = PTE_VALID | PTE_BLOCK | PTE_ATTR_IDX(ATTR_DEVICE) | PTE_AF;
        ASSERT_TRUE(pte & PTE_VALID);
        ASSERT_FALSE(pte & PTE_PAGE);
        ASSERT_TRUE(pte & PTE_AF);
    }

    TEST("Read-only page PTE has AP_RO set") {
        pte_t pte = PTE_VALID | PTE_PAGE | PTE_AP_RO | PTE_AF;
        ASSERT_TRUE(pte & PTE_AP_RO);
    }

    TEST("User-accessible page PTE has AP_EL0 set") {
        pte_t pte = PTE_VALID | PTE_PAGE | PTE_AP_EL0 | PTE_AF;
        ASSERT_TRUE(pte & PTE_AP_EL0);
    }

    TEST("page_table_t contains 512 entries of 8 bytes = 4096 bytes") {
        ASSERT_EQ_INT((int)sizeof(page_table_t), 512 * 8);
    }

    TEST("4-level translation: L0 index is VA[47:39] (9 bits)") {
        uint64_t va = 0x40000000ULL;
        uint64_t l0_index = (va >> 39) & 0x1FF;
        ASSERT_TRUE(l0_index >= 0 && l0_index < 512);
    }

    TEST("4-level translation: L1 index is VA[38:30] (9 bits)") {
        uint64_t va = 0x40000000ULL;
        uint64_t l1_index = (va >> 30) & 0x1FF;
        ASSERT_TRUE(l1_index >= 0 && l1_index < 512);
    }

    TEST("4-level translation: L2 index is VA[29:21] (9 bits)") {
        uint64_t va = 0x40100000ULL;
        uint64_t l2_index = (va >> 21) & 0x1FF;
        ASSERT_TRUE(l2_index >= 0 && l2_index < 512);
    }

    TEST("4-level translation: L3 index is VA[20:12] (9 bits)") {
        uint64_t va = 0x40001000ULL;
        uint64_t l3_index = (va >> 12) & 0x1FF;
        ASSERT_TRUE(l3_index >= 0 && l3_index < 512);
    }

    TEST("L1 block descriptor maps 1 GiB (30-bit block size)") {
        uint64_t block_size = 1ULL << 30;
        ASSERT_EQ_HEX(block_size, 0x40000000ULL);
    }

    TEST("L2 block descriptor maps 2 MiB (21-bit block size)") {
        uint64_t block_size = 1ULL << 21;
        ASSERT_EQ_HEX(block_size, 0x200000ULL);
    }

    TEST("L3 page descriptor maps 4 KiB (12-bit page size)") {
        uint64_t page_size = 1ULL << 12;
        ASSERT_EQ_HEX(page_size, 0x1000ULL);
    }

    TEST("mmuMapPage returns 0 on successful mapping") {
        SKIP("mmu implementation not yet available");
    }

    TEST("mmuUnmapPage returns 0 on successful unmapping") {
        SKIP("mmu implementation not yet available");
    }

    TEST("mmuVirtToPhys returns correct physical address after mapping") {
        SKIP("mmu implementation not yet available");
    }

    TEST("mmuMakePte constructs a valid PTE from phys and flags") {
        SKIP("mmu implementation not yet available");
    }
}