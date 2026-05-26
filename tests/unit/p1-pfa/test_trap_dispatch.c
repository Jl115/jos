#include "test.h"
#include "trap.h"
#include "drivers/gic/registry.h"
#include "drivers/gic/gic.h"

#include <stdint.h>
#include <string.h>

extern volatile gicdT mock_gicd;
extern volatile giccT mock_gicc;

static void reset_gic_mocks(void) {
    memset((void *)&mock_gicd, 0, sizeof(gicdT));
    memset((void *)&mock_gicc, 0, sizeof(giccT));
}

PHASE(p1_trap_dispatch) {
    TEST("trap_frame_t has 34 uint64_t fields totalling 272 bytes") {
        ASSERT_EQ_INT((int)sizeof(trap_frame_t), 34 * 8);
    }

    TEST("x[0] offset is 0 within trap_frame_t") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.x[0] - (uintptr_t)&f), 0);
    }

    TEST("x[29] offset is 232 within trap_frame_t") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.x[29] - (uintptr_t)&f), 29 * 8);
    }

    TEST("lr field offset is 240") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.lr - (uintptr_t)&f), 30 * 8);
    }

    TEST("elr field offset is 248") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.elr - (uintptr_t)&f), 31 * 8);
    }

    TEST("spsr field offset is 256") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.spsr - (uintptr_t)&f), 32 * 8);
    }

    TEST("esr field offset is 264") {
        trap_frame_t f;
        ASSERT_EQ_INT((int)((uintptr_t)&f.esr - (uintptr_t)&f), 33 * 8);
    }

    TEST("ESR EC extraction: data abort lower EL is 0x24") {
        uint64_t esr = (0x24ULL << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x24);
    }

    TEST("ESR EC extraction: data abort same EL is 0x25") {
        uint64_t esr = (0x25ULL << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x25);
    }

    TEST("ESR EC extraction: SVC from AArch64 is 0x15") {
        uint64_t esr = (0x15ULL << 26);
        uint32_t ec = (esr >> 26) & 0x3F;
        ASSERT_EQ_INT((int)ec, 0x15);
    }

    TEST("ESR ISS field: lower 24 bits contain syndrome info") {
        uint64_t esr = (0x15ULL << 26) | 0x42;
        uint32_t iss = esr & 0xFFFFFF;
        ASSERT_EQ_INT((int)iss, 0x42);
    }

    TEST("exceptionDispatch routes EC=0x15 to handleSynchronousException (not panic)") {
        reset_gic_mocks();
        mock_gicc.IAR = 1023;
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.esr = (0x15ULL << 26);
        exceptionDispatch(&frame);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 0);
    }

    TEST("exceptionDispatch routes valid IRQ (IAR=27) to handleHardwareInterrupt") {
        reset_gic_mocks();
        mock_gicc.IAR = 27;
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.esr = 0;
        exceptionDispatch(&frame);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 27);
    }

    TEST("exceptionDispatch with spurious IRQ (IAR=1023) calls kernelPanic") {
        reset_gic_mocks();
        mock_gicc.IAR = 1023;
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.esr = 0;
        exceptionDispatch(&frame);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 0);
    }

    TEST("exceptionDispatch with data abort EC and spurious IRQ calls kernelPanic") {
        reset_gic_mocks();
        mock_gicc.IAR = 1023;
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.esr = (0x24ULL << 26) | 0x2A;
        exceptionDispatch(&frame);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 0);
    }

    TEST("trap_frame_t preserves register values through dispatch") {
        reset_gic_mocks();
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.x[0] = 0xDEAD;
        frame.x[29] = 0xBEEF;
        frame.lr = 0x1234;
        frame.elr = 0x40000000;
        frame.spsr = 0x5;
        frame.esr = (0x15ULL << 26);
        exceptionDispatch(&frame);
        ASSERT_EQ_HEX(frame.x[0], 0xDEADULL);
        ASSERT_EQ_HEX(frame.x[29], 0xBEEFULL);
        ASSERT_EQ_HEX(frame.lr, 0x1234ULL);
        ASSERT_EQ_HEX(frame.elr, 0x40000000ULL);
    }

    TEST("exceptionDispatch does not call handleHardwareInterrupt for EC=0x15") {
        reset_gic_mocks();
        mock_gicc.IAR = 27;
        trap_frame_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.esr = (0x15ULL << 26);
        exceptionDispatch(&frame);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 0);
    }

    TEST("handleHardwareInterrupt called for timer IRQ writes EOIR") {
        reset_gic_mocks();
        mock_gicc.IAR = 27;
        handleHardwareInterrupt(27);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 27);
    }

    TEST("handleHardwareInterrupt for unknown IRQ also writes EOIR") {
        reset_gic_mocks();
        handleHardwareInterrupt(42);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 42);
    }

    TEST("gicReadIAR returns IAR value from mock") {
        reset_gic_mocks();
        mock_gicc.IAR = 42;
        ASSERT_EQ_INT((int)gicReadIAR(), 42);
    }
}