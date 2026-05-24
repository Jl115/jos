#include "drivers/uart/uart.h"
#include <stdint.h>

typedef struct {
    uint64_t x[30];
    uint64_t lr;
    uint64_t elr;
    uint64_t spsr;
    uint64_t esr;
} trap_frame_t;

void exception_dispatch(trap_frame_t *frame) {
    uint32_t ec = (frame->esr >> 26) & 0x3F;

    if (ec == 0x15) {
        uart_printf("\n\x1b[33m--- SYSCALL (SVC) DETECTED ---\x1b[0m\n");
        uart_printf("Address (ELR): 0x%x\n", frame->elr);
        /* Advance past the SVC instruction */
        frame->elr += 4;
        uart_printf("Resuming execution...\n\n");
        return;
    }
    uart_printf("\n\x1b[31m--- FATAL EXCEPTION ---\x1b[0m\n");
    uart_printf("ESR_EL1: 0x%x (EC: 0x%x)\n", frame->esr, ec);
    uart_printf("ELR_EL1: 0x%x\n", frame->elr);
    uart_printf("System Halted.\n");
    while (1) {
    };
}

static uint64_t read_cntfrq(void) {
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    uart_printf("\x1b[32mJOS Frequency is: %uHz\x1b[0m\n", freq);
    return freq;
}

static void init_driver_states(void) {
    uart_printf_init();
}

void kernel_main(void) {
    init_driver_states();
    uint64_t timer_freq = read_cntfrq();
    uart_printf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");

    __asm__ volatile("msr cntp_tval_el0, %0" ::"r"(timer_freq));
    __asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(1ULL));

    while (1) {
        __asm__ volatile("wfi");
    }
}
