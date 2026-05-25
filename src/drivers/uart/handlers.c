#include "handlers.h"
#include "uart.h"
#include <stdint.h>

void handleC(va_list *args) {
    uartPutc((char)va_arg(*args, int));
}
void handleS(va_list *args) {
    const char *s = va_arg(*args, const char *);
    uartPrint(s ? s : "(null)");
}
void handleD(va_list *args) {
    printSigned(va_arg(*args, int));
}
void handleX(va_list *args) {
    printUnsigned(va_arg(*args, unsigned int), 16);
}
void handleU(va_list *args) {
    printUnsigned(va_arg(*args, uint64_t), 10);
}
void handlePCT(va_list *args) {
    (void)args;
    uartPutc('%');
}

formatHandlerT formatHandlers[256] = {0};

void handleFormat(char specifier, va_list *args) {
    formatHandlerT handler = formatHandlers[(unsigned char)specifier];

    if (handler) {
        handler(args);
    } else {
        uartPutc('%');
        uartPutc(specifier);
    }
}
