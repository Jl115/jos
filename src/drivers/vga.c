#include "vga.h"

static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;
static uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;

static int terminal_row = 0;
static int terminal_column = 0;

void vga_clear(void) {
  for (int y = 0; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      // Leerzeichen ( ) in der Farbe Weiß (0x0F)
      VGA_MEMORY[y * VGA_WIDTH + x] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    }
  }
  terminal_row = 0;
  terminal_column = 0;
}

void vga_print(const char *str, uint8_t color) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      terminal_column = 0;
      terminal_row++;
      continue;
    }

    VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] =
        (uint16_t)str[i] | (uint16_t)color << 8;
    terminal_column++;

    if (terminal_column >= VGA_WIDTH) {
      terminal_column = 0;
      terminal_row++;
    }
  }
}
