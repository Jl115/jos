#include "handlers.h"
#include "uart.h"
#include <stdint.h>

void handle_c(va_list *args) {
    uart_putc((char)va_arg(*args, int));
}
void handle_s(va_list *args) {
    const char *s = va_arg(*args, const char *);
    uart_print(s ? s : "(null)");
}
void handle_d(va_list *args) {
    print_signed(va_arg(*args, int));
}
void handle_x(va_list *args) {
    print_unsigned(va_arg(*args, unsigned int), 16);
}
void handle_u(va_list *args) {
    print_unsigned(va_arg(*args, uint64_t), 10);
}
void handle_pct(va_list *args) {
    (void)args;
    uart_putc('%');
}

format_handler_t format_handlers[256] = {0};

void handle_format(char specifier, va_list *args) {
    format_handler_t handler = format_handlers[(unsigned char)specifier];

    if (handler) {
        handler(args);
    } else {
        uart_putc('%');
        uart_putc(specifier);
    }
}
