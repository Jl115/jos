#ifndef UART_H
#define UART_H

void uart_putc(char c);

void uart_print(const char *str);

void uart_printf(const char *format, ...);
#endif
