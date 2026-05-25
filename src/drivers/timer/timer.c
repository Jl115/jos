#include "timer/timer.h"
#include "drivers/gic/gic.h"
#include "uart/uart.h"
#include <stdint.h>

volatile uint64_t systemTicks = 0;

uint64_t readCntfrq(void) {
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

void timerHandleTick(uint32_t irq_id) {
    systemTicks++;
    uint64_t timer_freq = readCntfrq();
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(timer_freq / 1000));
    gicWriteEoir(irq_id);
}

void setVirtualTimer(uint64_t intervall) {
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(intervall));
    __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(1ULL));
}
