#include "uart.h"
#include "handlers.h"
#include "uart_registry.h"
#include <stdarg.h>
#include <stdint.h>

#define TX_FULL_FLAG (1 << 5)

/* --- Hardware Abstraction --- */
static inline void uartWaitTXReady(void) {
    while (UART0->FLAG_REGISTER & TX_FULL_FLAG) {
        // Wait for TX buffer to clear
    }
}

static inline void uartSendChar(char c) {
    uartWaitTXReady();
    UART0->DATA_REGISTER = (uint32_t)(unsigned char)c;
}

/* --- Public Core API --- */
void uartPutc(char c) {
    if (c == '\n') {
        uartSendChar('\r');
    }
    uartSendChar(c);
}

void uartPrint(const char *str) {
    while (*str) {
        uartPutc(*str++);
    }
}

/* --- Number Formatting Helpers --- */
void printUnsigned(uint64_t value, int base) {
    char buffer[64];
    int  i = 0;

    if (value == 0) {
        uartPutc('0');
        return;
    }

    while (value != 0) {
        uint64_t rem = value % base;
        buffer[i++]  = (char)((rem > 9) ? (rem - 10) + 'a' : rem + '0');
        value /= base;
    }

    while (i > 0) {
        uartPutc(buffer[--i]);
    }
}

void printSigned(int value) {
    if (value >= 0) {
        printUnsigned((uint64_t)value, 10);
        return;
    }
    uartPutc('-');
    printUnsigned((uint64_t)(-value), 10);
}

void uartPrintfInit(void) {
    formatHandlers['c'] = handleC;
    formatHandlers['s'] = handleS;
    formatHandlers['d'] = handleD;
    formatHandlers['x'] = handleX;
    formatHandlers['u'] = handleU;
    formatHandlers['%'] = handlePCT;
}

/* --- Printf Implementation --- */
void uartPrintf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            uartPutc(*format++);
            continue;
        }

        format++; // Skip the '%'

        if (*format == '\0') {
            break;
        }

        handleFormat(*format++, &args);
    }

    va_end(args);
}
