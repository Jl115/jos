#include "uart.h"
#include "handlers.h"
#include "uart_registry.h"
#include <stdarg.h>
#include <stdint.h>

#define TX_FULL_FLAG (1 << 5)

/* --- Hardware Abstraction --- */
static inline void uart_wait_tx_ready(void) {
    while (UART0->FLAG_REGISTER & TX_FULL_FLAG) {
        // Wait for TX buffer to clear
    }
}

static inline void uart_send_char(char c) {
    uart_wait_tx_ready();
    UART0->DATA_REGISTER = (uint32_t)(unsigned char)c;
}

/* --- Public Core API --- */
void uart_putc(char c) {
    if (c == '\n') {
        uart_send_char('\r');
    }
    uart_send_char(c);
}

void uart_print(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

/* --- Number Formatting Helpers --- */
void print_unsigned(uint64_t value, int base) {
    char buffer[64];
    int  i = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value != 0) {
        uint64_t rem = value % base;
        buffer[i++]  = (char)((rem > 9) ? (rem - 10) + 'a' : rem + '0');
        value /= base;
    }

    while (i > 0) {
        uart_putc(buffer[--i]);
    }
}

void print_signed(int value) {
    if (value >= 0) {
        print_unsigned((uint64_t)value, 10);
        return;
    }
    uart_putc('-');
    print_unsigned((uint64_t)(-value), 10);
}

/* YOU MUST CALL THIS IN kernel_main BEFORE PRINTING VARIABLES */
void uart_printf_init(void) {
    format_handlers['c'] = handle_c;
    format_handlers['s'] = handle_s;
    format_handlers['d'] = handle_d;
    format_handlers['x'] = handle_x;
    format_handlers['u'] = handle_u;
    format_handlers['%'] = handle_pct;
}

/* --- Printf Implementation --- */
void uart_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            uart_putc(*format++);
            continue;
        }

        format++; // Skip the '%'

        if (*format == '\0') {
            break;
        }

        handle_format(*format++, &args);
    }

    va_end(args);
}
