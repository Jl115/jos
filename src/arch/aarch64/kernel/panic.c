#include "arch/aarch64/include/trap.h"
#include "drivers/uart/uart.h"

void kernel_panic(trap_frame_t *frame, uint32_t ec) {
    uart_printf("\n\x1b[31m--- FATAL EXCEPTION ---\x1b[0m\n");
    uart_printf("ESR_EL1: 0x%x (EC: 0x%x)\n", frame->esr, ec);
    uart_printf("ELR_EL1: 0x%x\n", frame->elr);
    uart_printf("System Halted.\n");
    while (1) {
        __asm__ volatile("wfe"); // wfe is better than an empty loop to save power
    };
}
