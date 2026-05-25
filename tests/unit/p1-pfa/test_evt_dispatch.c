/* ============================================================================
 * Phase 1: Exception Vector Table
 * Tests for interrupt handler dispatch logic and ESR decoding algorithms.
 *
 * These tests exercise the C dispatch paths without requiring QEMU or
 * the actual AArch64 EVT assembly.  No MMIO, no traps.
 * ============================================================================ */

#include "test.h"

/* We can't compile exeption.S / vectors.S on the host, so we test the
 * dispatch *algorithm* by mocking the parts that exercise trap frames.
 */

typedef struct {
    uint64_t x[30];
    uint64_t lr;
    uint64_t elr;
    uint64_t spsr;
    uint64_t esr;
} trap_frame_test_t;

/* ESR_EL1.EC field (bits [31:26]) */
#define ESR_EC_SHIFT 26
#define ESR_EC_MASK  0x3F
#define ESR_GET_EC(esr) (((esr) >> ESR_EC_SHIFT) & ESR_EC_MASK)

static void esr_decode_return(trap_frame_test_t *frame) {
    (void)frame;
    /* stub for synchronous handler */
}

static void esr_decode_irq(trap_frame_test_t *frame) {
    (void)frame;
    /* stub for IRQ handler */
}

static void esr_decode_panic(trap_frame_test_t *frame) {
    (void)frame;
    /* stub for panic */
}

/* Mirror the dispatch logic in main.c's exceptionDispatch() */
static int dispatch_ec(uint64_t esr) {
    uint32_t ec = ESR_GET_EC(esr);
    if (ec == 0x15) return 1; /* syscall */
    if (ec == 0x24 || ec == 0x25) return 2; /* data abort */
    if (ec == 0x20 || ec == 0x21) return 3; /* instruction abort */
    if (ec == 0x26) return 4; /* SP alignment fault */
    if (ec == 0x32) return 5; /* BRK */
    return 0; /* unknown */
}

/* ========================================================================= */
PHASE(p1_exception_vector_table) {

    TEST("ESR_EC extraction for syscall (0x15)") {
        uint64_t esr = ((uint64_t)0x15) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 1);
    }

    TEST("ESR_EC extraction for data abort lower (0x24)") {
        uint64_t esr = ((uint64_t)0x24) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 2);
    }

    TEST("ESR_EC extraction for data abort current (0x25)") {
        uint64_t esr = ((uint64_t)0x25) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 2);
    }

    TEST("ESR_EC extraction for instruction abort (0x20)") {
        uint64_t esr = ((uint64_t)0x20) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 3);
    }

    TEST("ESR_EC extraction for SP alignment fault (0x26)") {
        uint64_t esr = ((uint64_t)0x26) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 4);
    }

    TEST("ESR_EC extraction for BRK (0x32)") {
        uint64_t esr = ((uint64_t)0x32) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 5);
    }

    TEST("Unknown EC returns 0") {
        uint64_t esr = ((uint64_t)0x00) << ESR_EC_SHIFT;
        ASSERT_EQ_INT(dispatch_ec(esr), 0);
    }

    TEST("ESR extraction with non-zero ISS field (lower bits)") {
        uint64_t esr = (((uint64_t)0x15) << ESR_EC_SHIFT) | 0x1234;
        ASSERT_EQ_INT(dispatch_ec(esr), 1);
    }

    TEST("Trap frame size is correct (should be 34 * 8 bytes)") {
        ASSERT_EQ_INT(sizeof(trap_frame_test_t), 34 * sizeof(uint64_t));
    }

    TEST("ESR_EC mask does not depend on higher bits") {
        uint64_t esr = (((uint64_t)0x24) << ESR_EC_SHIFT) | (1ULL << 63);
        ASSERT_EQ_INT(dispatch_ec(esr), 2);
    }
}
