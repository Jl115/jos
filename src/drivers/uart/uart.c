#include "./uart.h"
#include "./uart_registry.h"

void uart_putc(char c) {
    while (UART0->FLAG_REGISTER & (1 << 5)) {
    }

    UART0->DATA_REGISTER = (uint32_t)(unsigned char)c;
}

void uart_print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_putc(str[i]);
    }
}
