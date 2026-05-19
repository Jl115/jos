#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void vga_clear(void);

void vga_print(const char *str, uint8_t color);

#endif
