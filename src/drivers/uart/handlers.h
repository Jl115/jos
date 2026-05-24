#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdarg.h>

/* Export the type definition */
typedef void (*format_handler_t)(va_list *args);

/* Export the specific handlers */
void handle_c(va_list *args);
void handle_s(va_list *args);
void handle_d(va_list *args);
void handle_x(va_list *args);
void handle_u(va_list *args);
void handle_pct(va_list *args);
void handle_format(char specifier, va_list *args);

/* Export the Dispatch Table globally */
extern format_handler_t format_handlers[256];

#endif
