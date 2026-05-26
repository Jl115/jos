#include "drivers/fdt/fdt.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"
#include "drivers/uart/uart.h"
#include "mm/pfa.h"
#include "trap.h"
#include <stdint.h>

extern uint32_t          endkernel;
extern volatile uint64_t systemTicks;

static void initDriverStates(void) {
    uartPrintfInit();
}

static void enableCpuIrq(void) {
#ifndef TEST_BUILD
    __asm__ volatile("msr daifclr, #2");
#endif
}

void kernelMain(uint64_t dtb_ptr) {
    initDriverStates();
    uartPrintf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");

    parseMemoryFromFdt(dtb_ptr);

    if (npages == 0) {
        uartPrintf("PANIC: Failed to find /memory node in FDT.\n");
        while (1) {
        }
    }

    pageframeT p1 = kallocFrame();
    pageframeT p2 = kallocFrame();
    uartPrintf("Allocated P1: 0x%016llx\n", p1);
    uartPrintf("Allocated P2: 0x%016llx\n", p2);

    if (p1 == ERROR || p2 == ERROR || p1 == p2) {
        uartPrintf("PANIC: Allocator is broken.\n");
        while (1)
            ;
    }

    kfreeFrame(p1);
    pageframeT p3 = kallocFrame();
    uartPrintf("Freed P1, Allocated P3: 0x%016llx\n", p3);

    if (p1 == p3) {
        uartPrintf("PFA Test: PASS (Frame Reused)\n\n");
    } else {
        uartPrintf("PFA Test: FAIL (Frame Not Reused)\n\n");
        while (1)
            ;
    }

    dumpFdt(dtb_ptr);

    initGIC();
    uint64_t timer_freq = readCntfrq();
    setVirtualTimer(timer_freq / 1000);

    enableCpuIrq();

    uint64_t last_tick = 0;

    while (1) {
        if (systemTicks != last_tick) {
            last_tick = systemTicks;
        }
#ifndef TEST_BUILD
        __asm__ volatile("wfi");
#endif
    }
}