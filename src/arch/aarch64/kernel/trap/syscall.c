#include "arch/aarch64/include/trap.h"
#include "drivers/uart/uart.h"

void handleSynchronousException(trap_frame_t *frame) {
#ifndef TEST_BUILD
    uartPrintf("\n\x1b[33m--- SYSCALL (SVC) DETECTED ---\x1b[0m\n");
    uartPrintf("Address (ELR): 0x%x\n", frame->elr);
    frame->elr += 4;
    uartPrintf("Resuming execution...\n\n");
#else
    (void)frame;
#endif
}
