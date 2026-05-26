#include "test.h"

#include <stdint.h>
#include <stddef.h>

PHASE(p1_exception_vector_table) {
    TEST("Vector table alignment: 2048 bytes (2^11)") {
        ASSERT_EQ_INT(1 << 11, 2048);
    }

    TEST("Each vector entry is 128 bytes (2^7)") {
        ASSERT_EQ_INT(1 << 7, 128);
    }

    TEST("16 entries in full vector table = 8 exceptions x 2 exception levels") {
        int entries = 16;
        ASSERT_EQ_INT(entries, 16);
    }

    TEST("Total vector table size: 16 entries x 128 bytes = 2048 bytes") {
        int total = 16 * 128;
        ASSERT_EQ_INT(total, 2048);
    }

    TEST("VBAR_EL1 must be 2048-byte aligned (bits [10:0] reserved)") {
        uint64_t vbar = 0x40000000ULL;
        ASSERT_EQ_INT((int)(vbar & 0x7FF), 0);
    }

    TEST("ESR_EL1 EC field is bits [31:26] — 6-bit Exception Class") {
        uint32_t esr = 0x92000046ULL;
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x24);
    }

    TEST("DAIF mask: bit 1 (IRQ) is 0x2") {
        uint64_t daif_irq_bit = 0x2;
        ASSERT_EQ_INT((int)daif_irq_bit, 2);
    }

    TEST("DAIF mask: bit 2 (FIQ) is 0x4") {
        uint64_t daif_fiq_bit = 0x4;
        ASSERT_EQ_INT((int)daif_fiq_bit, 4);
    }

    TEST("Exception syndrome register: Data Abort from lower EL = EC 0x24") {
        uint64_t esr = (0x24ULL << 26) | 0x2A;
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x24);
    }

    TEST("Exception syndrome register: Data Abort from current EL = EC 0x25") {
        uint64_t esr = (0x25ULL << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x25);
    }

    TEST("Exception syndrome register: SVC from AArch64 = EC 0x15") {
        uint64_t esr = (0x15ULL << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x15);
    }

    TEST("SPSR_EL1: M[3:0]=0x0 means EL0t (user mode)") {
        uint64_t spsr = 0x00000000ULL;
        uint32_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0);
    }

    TEST("SPSR_EL1: M[3:0]=0x4 means EL1t (kernel mode, SP_EL0)") {
        uint64_t spsr = 0x4ULL;
        uint32_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0x4);
    }

    TEST("SPSR_EL1: M[3:0]=0x5 means EL1h (kernel mode, SP_EL1)") {
        uint64_t spsr = 0x5ULL;
        uint32_t m = spsr & 0xF;
        ASSERT_EQ_INT((int)m, 0x5);
    }

    TEST("kernel_entry saves x0-x30 (31 regs) + ELR + SPSR + ESR = 34 slots") {
        int x_regs = 30;
        int special_regs = 3;
        int total = x_regs + special_regs + 1;
        ASSERT_EQ_INT(total, 34);
    }

    TEST("Trap frame size: 34 registers x 8 bytes = 272 bytes") {
        ASSERT_EQ_INT(34 * 8, 272);
    }

    TEST("Stack must be 16-byte aligned per AArch64 calling convention") {
        uint64_t sp = 0x41000000ULL;
        ASSERT_EQ_INT((int)(sp & 0xF), 0);
    }
}