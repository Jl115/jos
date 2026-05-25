#include "timer/timer.h"
#include "drivers/gic/gic.h"
#include "uart/uart.h"
#include <stdint.h>

volatile uint64_t systemTicks = 0;

uint64_t readCntfrq(void) {
#ifndef TEST_BUILD
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
#else
    /* QEMU virt typical frequency; used by host tests */
    return 62500000ULL;
#endif
}
void timerHandleTick(uint32_t irq_id) {
    systemTicks++;
    (void)gicReadIAR(); /* just to keep the call chain present */
#ifndef TEST_BUILD
    uint64_t timer_freq __attribute__((unused)) = readCntfrq();
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(timer_freq / 1000));
#endif
    gicWriteEoir(irq_id);
}

void setVirtualTimer(uint64_t interval) {
#ifndef TEST_BUILD
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(interval));
    __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(1ULL));
#endif
}
