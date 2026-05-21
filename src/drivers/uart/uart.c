#include "./uart.h"
#include "./uart_registry.h"
#include <stdarg.h>

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
static void print_unsigned(unsigned int value, int base) {
    char buffer[32];
    int  i = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value != 0) {
        unsigned int rem = value % base;
        buffer[i++]      = (char)((rem > 9) ? (rem - 10) + 'a' : rem + '0');
        value /= base;
    }

    while (i > 0) {
        uart_putc(buffer[--i]);
    }
}

static void print_signed(int value) {
    if (value < 0) {
        uart_putc('-');
        print_unsigned((unsigned int)(-value), 10);
    } else {
        print_unsigned((unsigned int)value, 10);
    }
}

/* --- Printf Implementation --- */
static void handle_format(char specifier, va_list *args) {
    switch (specifier) {
    case 'c':
        uart_putc((char)va_arg(*args, int));
        break;
    case 's': {
        const char *s = va_arg(*args, const char *);
        uart_print(s ? s : "(null)");
        break;
    }
    case 'd':
        print_signed(va_arg(*args, int));
        break;
    case 'x':
        print_unsigned(va_arg(*args, unsigned int), 16);
        break;
    case '%':
        uart_putc('%');
        break;
    default:
        uart_putc('%');
        uart_putc(specifier);
        break;
    }
}

void uart_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == '\0') {
                break;
            }
            handle_format(*format, &args);
        } else {
            uart_putc(*format);
        }
        format++;
    }

    va_end(args);
}
