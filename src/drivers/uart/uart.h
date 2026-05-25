#ifndef UART_H
#define UART_H

#include <stdint.h>
void uart_putc(char c);

void uart_print(const char *str);

void uart_printf(const char *format, ...);

void uart_printf_init(void);

void print_unsigned(uint64_t value, int base);

void print_signed(int value);

#endif
