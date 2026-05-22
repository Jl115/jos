#include "drivers/uart/uart.h"

void kernel_main(void) {
    int ram_start     = 0x40000000;
    int test_negative = -42;

    uart_printf("RAM Start Adress: 0x%x\n", ram_start);
    uart_printf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");
    uart_printf("Negative Number (%s): %d\n", "Decimal", test_negative);
    uart_printf("One Char: %c\n", 'Z');

    while (1) {
    }
}
