#include "timer/timer.h"
#include "drivers/gic/gic.h"
#include "uart/uart.h"
#include <stdint.h>

volatile uint64_t system_ticks = 0;

uint64_t read_cntfrq(void) {
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

void timer_handle_tick(uint32_t irq_id) {
    system_ticks++;
    uint64_t timer_freq = read_cntfrq();
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(timer_freq / 1000));
    gic_write_eoir(irq_id);
}

void set_virtual_timer(uint64_t intervall) {
    __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(intervall));
    __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(1ULL));
}
