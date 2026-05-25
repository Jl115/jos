#include "arch/aarch64/include/trap.h"
#include "drivers/uart/uart.h"

void kernelPanic(trap_frame_t *frame, uint32_t ec) {
    uartPrintf("\n\x1b[31m--- FATAL EXCEPTION ---\x1b[0m\n");
    uartPrintf("ESR_EL1: 0x%x (EC: 0x%x)\n", frame->esr, ec);
    uartPrintf("ELR_EL1: 0x%x\n", frame->elr);
    uartPrintf("System Halted.\n");
    while (1) {
        __asm__ volatile("wfe"); // wfe is better than an empty loop to save power
    };
}
