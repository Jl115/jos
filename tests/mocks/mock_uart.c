/* ============================================================================
 * Mock UART layer for host-side testing.
 * Buffers uartPrintf output in memory instead of writing to non-existent MMIO.
 * ============================================================================ */
#include "drivers/uart/uart.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

static char g_uart_buffer[4096];
static int  g_uart_pos = 0;
static int  g_uart_initialized = 0;

/* Internal printf-style formatter stored in uart.c; we just capture the
 * final formatted bytes here.
 */

void mockUartReset(void) {
    memset(g_uart_buffer, 0, sizeof(g_uart_buffer));
    g_uart_pos = 0;
    g_uart_initialized = 0;
}

const char *mockUartGetBuffer(void) {
    return g_uart_buffer;
}

/* uartPrintf handlers call uartPutc; implementing a simple uartPrintf that
 * routes to the host stderr for visibility during test failures.
 */
void uartPutc(char c) {
    (void)c;
}

void uartPrint(const char *str) {
    (void)str;
}

void uartPrintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    /* Send to host stderr so test failures are visible */
    vfprintf(stderr, format, args);
    va_end(args);
}

void uartPrintfInit(void) {
    static char buf[1];
    (void)buf;
    g_uart_initialized = 1;
}

void printUnsigned(uint64_t value, int base) {
    char buffer[64];
    int  i = 0;
    if (value == 0) {
        uartPutc('0');
        return;
    }
    while (value != 0) {
        uint64_t rem = value % (uint64_t)base;
        buffer[i++] = (char)((rem > 9) ? (rem - 10) + 'a' : rem + '0');
        value /= (uint64_t)base;
    }
    while (i > 0) { uartPutc(buffer[--i]); }
}

void printSigned(int value) {
    if (value >= 0) { printUnsigned((uint64_t)value, 10); return; }
    uartPutc('-');
    printUnsigned((uint64_t)(-value), 10);
}

/* Printf format handlers define a dispatch table in handlers.c.
 * On host we stub these so the kernel handlers.c can still compile
 * without touching UART MMIO.
 */
