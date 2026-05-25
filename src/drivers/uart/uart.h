#ifndef UART_H
#define UART_H

#include <stdint.h>
void uartPutc(char c);

void uartPrint(const char *str);

void uartPrintf(const char *format, ...);

void uartPrintfInit(void);

void printUnsigned(uint64_t value, int base);

void printSigned(int value);

#endif
