#include "test.h"
#include "drivers/gic/registry.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"

#include <stdint.h>
#include <string.h>

extern volatile gicdT mock_gicd;
extern volatile giccT mock_gicc;

static void reset_gic_mocks(void) {
    memset((void *)&mock_gicd, 0, sizeof(gicdT));
    memset((void *)&mock_gicc, 0, sizeof(giccT));
}

PHASE(p2_gic_and_timers) {
    TEST("initGIC sets GICD CTLR to 1 (enable)") {
        reset_gic_mocks();
        initGIC();
        ASSERT_EQ_INT((int)mock_gicd.CTLR, 1);
    }

    TEST("initGIC sets GICC CTLR to 1 (enable)") {
        reset_gic_mocks();
        initGIC();
        ASSERT_EQ_INT((int)mock_gicc.CTLR, 1);
    }

    TEST("initGIC sets GICC PMR to 0xFFFF (allow all priorities)") {
        reset_gic_mocks();
        initGIC();
        ASSERT_EQ_INT((int)mock_gicc.PMR, 0xFFFF);
    }

    TEST("initGIC enables TIMER_IRQ (27) in ISENABLER") {
        reset_gic_mocks();
        initGIC();
        uint32_t reg = mock_gicd.ISENABLER[TIMER_IRQ / 32];
        uint32_t expected_bit = (1 << (TIMER_IRQ % 32));
        ASSERT_TRUE(reg & expected_bit);
    }

    TEST("initGIC targets TIMER_IRQ to CPU 0 in ITARGETSR") {
        reset_gic_mocks();
        initGIC();
        ASSERT_EQ_INT((int)mock_gicd.ITARGETSR[TIMER_IRQ], 1);
    }

    TEST("initGIC disables distributor before setup (CTLR=0)") {
        reset_gic_mocks();
        mock_gicd.CTLR = 0xFF;
        initGIC();
        ASSERT_EQ_INT((int)mock_gicd.CTLR, 1);
    }

    TEST("gicReadIAR reads from GICC IAR register") {
        reset_gic_mocks();
        mock_gicc.IAR = 27;
        uint32_t result = gicReadIAR();
        ASSERT_EQ_INT((int)result, 27);
    }

    TEST("gicReadIAR returns spurious IRQ ID 1023") {
        reset_gic_mocks();
        mock_gicc.IAR = 1023;
        uint32_t result = gicReadIAR();
        ASSERT_EQ_INT((int)result, 1023);
    }

    TEST("gicWriteEoir writes IRQ ID to GICC EOIR register") {
        reset_gic_mocks();
        gicWriteEoir(27);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 27);
    }

    TEST("gicWriteEoir writes spurious ID 1023") {
        reset_gic_mocks();
        gicWriteEoir(1023);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 1023);
    }

    TEST("TIMER_IRQ constant is 27") {
        ASSERT_EQ_INT((int)TIMER_IRQ, 27);
    }

    TEST("GICv2 ISENABLER register index for TIMER_IRQ = 27/32 = 0") {
        int reg_index = TIMER_IRQ / 32;
        ASSERT_EQ_INT(reg_index, 0);
    }

    TEST("GICv2 ISENABLER bit position for TIMER_IRQ = 27 % 32 = 27") {
        int bit_pos = TIMER_IRQ % 32;
        ASSERT_EQ_INT(bit_pos, 27);
    }

    TEST("readCntfrq returns QEMU virt frequency 62500000") {
        uint64_t freq = readCntfrq();
        ASSERT_EQ_INT((int64_t)freq, 62500000LL);
    }

    TEST("systemTicks starts at 0") {
#if defined(TEST_BUILD)
        systemTicks = 0;
#else
        ASSERT_EQ_INT((int)systemTicks, 0);
#endif
        systemTicks = 0;
        ASSERT_EQ_INT((int)systemTicks, 0);
    }

    TEST("timerHandleTick increments systemTicks") {
        systemTicks = 0;
        reset_gic_mocks();
        timerHandleTick(27);
        ASSERT_EQ_INT((int)systemTicks, 1);
    }

    TEST("timerHandleTick increments systemTicks multiple times") {
        systemTicks = 0;
        reset_gic_mocks();
        timerHandleTick(27);
        timerHandleTick(27);
        timerHandleTick(27);
        ASSERT_EQ_INT((int)systemTicks, 3);
    }

    TEST("timerHandleTick calls gicWriteEoir with the same IRQ ID") {
        reset_gic_mocks();
        timerHandleTick(27);
        ASSERT_EQ_INT((int)mock_gicc.EOIR, 27);
    }

    TEST("GICD struct has CTLR at offset 0x000") {
        gicdT g;
        ASSERT_EQ_INT((int)((uintptr_t)&g.CTLR - (uintptr_t)&g), 0);
    }

    TEST("GICD struct has ISENABLER at offset 0x100") {
        gicdT g;
        ASSERT_EQ_INT((int)((uintptr_t)&g.ISENABLER - (uintptr_t)&g), 0x100);
    }

    TEST("GICD struct has ITARGETSR at offset 0x800") {
        gicdT g;
        ASSERT_EQ_INT((int)((uintptr_t)&g.ITARGETSR - (uintptr_t)&g), 0x800);
    }

    TEST("GICC struct has CTLR at offset 0x000") {
        giccT c;
        ASSERT_EQ_INT((int)((uintptr_t)&c.CTLR - (uintptr_t)&c), 0);
    }

    TEST("GICC struct has PMR at offset 0x004") {
        giccT c;
        ASSERT_EQ_INT((int)((uintptr_t)&c.PMR - (uintptr_t)&c), 0x4);
    }

    TEST("GICC struct has IAR at offset 0x00C") {
        giccT c;
        ASSERT_EQ_INT((int)((uintptr_t)&c.IAR - (uintptr_t)&c), 0xC);
    }

    TEST("GICC struct has EOIR at offset 0x010") {
        giccT c;
        ASSERT_EQ_INT((int)((uintptr_t)&c.EOIR - (uintptr_t)&c), 0x10);
    }
}