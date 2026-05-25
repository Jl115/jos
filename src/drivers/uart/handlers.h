#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdarg.h>

/* Export the type definition */
typedef void (*formatHandlerT)(va_list *args);

/* Export the specific handlers */
void handleC(va_list *args);
void handleS(va_list *args);
void handleD(va_list *args);
void handleX(va_list *args);
void handleU(va_list *args);
void handlePCT(va_list *args);
void handleFormat(char specifier, va_list *args);

/* Export the Dispatch Table globally */
extern formatHandlerT formatHandlers[256];

#endif
