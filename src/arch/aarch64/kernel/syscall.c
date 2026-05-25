#include "arch/aarch64/include/trap.h"
#include "drivers/uart/uart.h"

void handle_synchronous_exception(trap_frame_t *frame) {
    uart_printf("\n\x1b[33m--- SYSCALL (SVC) DETECTED ---\x1b[0m\n");
    uart_printf("Address (ELR): 0x%x\n", frame->elr);
    frame->elr += 4;
    uart_printf("Resuming execution...\n\n");
}
