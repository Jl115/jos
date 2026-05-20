#include "drivers/uart/uart.h"

void kernel_main(void) {
    uart_print("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");
    uart_print("\x1b[37mModule 1: UART Text Driver Loaded.\n\x1b[0m");
    uart_print("\x1b[31mSystem Ready.\n\x1b[0m");

    while (1) {
    }
}
