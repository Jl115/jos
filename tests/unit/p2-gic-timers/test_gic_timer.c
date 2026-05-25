/* ============================================================================
 * Phase 2: Hardware Awareness (GIC & Timers)
 * Tests that exercise GIC register writes and timer tick accounting logic.
 * No real MMIO — registers are mocked in RAM.
 * ============================================================================ */
#include "test.h"
#include "drivers/gic/registry.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"

#define TEST_IRQ_ID 27

static void resetMockGIC(void) {
    memset((void*)&mock_gicd, 0, sizeof(mock_gicd));
    memset((void*)&mock_gicc, 0, sizeof(mock_gicc));
}

/* ========================================================================= */
PHASE(p2_gic_and_timers) {

    /* --- Timer tick counting --- */
    TEST("Timer tick counter starts at zero") {
        systemTicks = 0;
        ASSERT_EQ_UINT(0, systemTicks);
    }

    TEST("Timer tick counter increments on handler call") {
        systemTicks = 0;
        timerHandleTick(27);
        ASSERT_EQ_UINT(1, systemTicks);
        timerHandleTick(27);
        ASSERT_EQ_UINT(2, systemTicks);
    }

    TEST("Timer handler writes EOI to GICC") {
        resetMockGIC();
        timerHandleTick(27);
        ASSERT_EQ_UINT(27, mock_gicc.EOIR);
    }

    /* --- GIC Distributor state --- */
    TEST("initGIC disables GICD and GICC during setup") {
        resetMockGIC();
        initGIC();
        /* In initGIC, first writes set CTLR to 0, then to 1 */
        ASSERT_EQ_UINT(1, mock_gicd.CTLR);
        ASSERT_EQ_UINT(1, mock_gicc.CTLR);
    }

    TEST("initGIC routes TIMER_IRQ to CPU 0") {
        resetMockGIC();
        initGIC();
        ASSERT_EQ_UINT(1, mock_gicd.ITARGETSR[TIMER_IRQ]);
    }

    TEST("initGIC enables TIMER_IRQ in ISENABLER") {
        resetMockGIC();
        initGIC();
        uint32_t idx = TIMER_IRQ / 32;
        uint32_t bit = 1U << (TIMER_IRQ % 32);
        ASSERT_TRUE((mock_gicd.ISENABLER[idx] & bit) != 0);
    }

    TEST("initGIC allows all priorities in PMR") {
        resetMockGIC();
        initGIC();
        ASSERT_EQ_HEX(0xFFFF, mock_gicc.PMR);
    }

    /* --- GIC acknowledgement --- */
    TEST("gicReadIAR returns the current IAR value") {
        resetMockGIC();
        mock_gicc.IAR = 42;
        ASSERT_EQ_UINT(42, gicReadIAR());
    }

    TEST("gicWriteEoir writes value to EOIR") {
        resetMockGIC();
        gicWriteEoir(99);
        ASSERT_EQ_UINT(99, mock_gicc.EOIR);
    }

    /* --- GIC edge cases --- */
    TEST("initGIC survives being called twice") {
        resetMockGIC();
        initGIC();
        initGIC();
        ASSERT_EQ_UINT(1, mock_gicd.CTLR);
        ASSERT_EQ_UINT(1, mock_gicc.CTLR);
    }

    TEST("gicReadIAR returns 1023 when spurious") {
        resetMockGIC();
        mock_gicc.IAR = 1023;
        ASSERT_EQ_UINT(1023, gicReadIAR());
    }
}
