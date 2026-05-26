#include "test.h"
#include "drivers/fdt/fdt.h"

#include <stdint.h>
#include <string.h>

PHASE(p3_fdt_and_pfa) {
    TEST("fdt32TOCpu byte-swaps 0xD00DFEED to 0xEDFE0DD0") {
        uint32_t result = fdt32TOCpu(0xD00DFEED);
        uint32_t expected = 0xEDFE0DD0;
        ASSERT_EQ_HEX((uint64_t)result, (uint64_t)expected);
    }

    TEST("fdt32TOCpu byte-swaps 0x00000100") {
        uint32_t result = fdt32TOCpu(0x00000100);
        ASSERT_EQ_HEX((uint64_t)result, (uint64_t)0x00010000ULL);
    }

    TEST("fdt32TOCpu is idempotent for palindromic values") {
        uint32_t val = 0x12341234;
        uint32_t double_swapped = fdt32TOCpu(fdt32TOCpu(val));
        ASSERT_EQ_HEX((uint64_t)double_swapped, (uint64_t)val);
    }

    TEST("fdt32TOCpu double-swap returns original") {
        uint32_t val = 0xDEADBEEF;
        uint32_t result = fdt32TOCpu(fdt32TOCpu(val));
        ASSERT_EQ_HEX((uint64_t)result, (uint64_t)val);
    }

    TEST("fdtStrcmp matches for identical strings") {
        ASSERT_EQ_INT(fdtStrcmp("memory", "memory"), 0);
    }

    TEST("fdtStrcmp detects mismatch at first character") {
        int result = fdtStrcmp("memory", "nemory");
        ASSERT_TRUE(result != 0);
    }

    TEST("fdtStrcmp detects mismatch at last character") {
        int result = fdtStrcmp("memory", "memorz");
        ASSERT_TRUE(result != 0);
    }

    TEST("fdtStrcmp returns negative when s1 < s2") {
        int result = fdtStrcmp("abc", "abd");
        ASSERT_TRUE(result < 0);
    }

    TEST("fdtStrcmp returns positive when s1 > s2") {
        int result = fdtStrcmp("abd", "abc");
        ASSERT_TRUE(result > 0);
    }

    TEST("fdtStrcmp handles empty strings") {
        ASSERT_EQ_INT(fdtStrcmp("", ""), 0);
    }

    TEST("fdtStrcmp handles one empty string") {
        int result = fdtStrcmp("", "a");
        ASSERT_TRUE(result < 0);
    }

    TEST("FDT_MAGIC constant is 0xd00dfeed") {
        ASSERT_EQ_HEX((uint64_t)FDT_MAGIC, 0xd00dfeedULL);
    }

    TEST("FDT token constants have correct values") {
        ASSERT_EQ_INT((int)FDT_BEGIN_NODE, 0x1);
        ASSERT_EQ_INT((int)FDT_END_NODE, 0x2);
        ASSERT_EQ_INT((int)FDT_PROP, 0x3);
        ASSERT_EQ_INT((int)FDT_NOP, 0x4);
        ASSERT_EQ_INT((int)FDT_END, 0x9);
    }

    TEST("FDT_ALIGN rounds up to 4-byte boundary") {
        uint8_t *ptr = (uint8_t *)5;
        uint32_t *aligned = FDT_ALIGN(ptr);
        ASSERT_EQ_INT((int)((uintptr_t)aligned & 3), 0);
        ASSERT_TRUE((uintptr_t)aligned >= (uintptr_t)ptr);
    }

    TEST("FDT_ALIGN on already-aligned pointer is a no-op") {
        uint8_t *ptr = (uint8_t *)8;
        uint32_t *aligned = FDT_ALIGN(ptr);
        ASSERT_EQ_INT((int)(uintptr_t)aligned, 8);
    }

    TEST("FDT_ALIGN on address 1 rounds to 4") {
        uint8_t *ptr = (uint8_t *)1;
        uint32_t *aligned = FDT_ALIGN(ptr);
        ASSERT_EQ_INT((int)(uintptr_t)aligned, 4);
    }

    TEST("FDT_ALIGN on address 3 rounds to 4") {
        uint8_t *ptr = (uint8_t *)3;
        uint32_t *aligned = FDT_ALIGN(ptr);
        ASSERT_EQ_INT((int)(uintptr_t)aligned, 4);
    }

    TEST("fdtHeader is packed (no padding)") {
        ASSERT_EQ_INT((int)sizeof(struct fdtHeader), 40);
    }

    TEST("fdtHeader magic field offset is 0") {
        struct fdtHeader h;
        ASSERT_EQ_INT((int)((uintptr_t)&h.magic - (uintptr_t)&h), 0);
    }

    TEST("fdtHeader totalsize field offset is 4") {
        struct fdtHeader h;
        ASSERT_EQ_INT((int)((uintptr_t)&h.totalsize - (uintptr_t)&h), 4);
    }

    TEST("fdtHeader offDTStruct field offset is 8") {
        struct fdtHeader h;
        ASSERT_EQ_INT((int)((uintptr_t)&h.offDTStruct - (uintptr_t)&h), 8);
    }
}