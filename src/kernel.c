#include "drivers/vga.h"

void kernel_main(void) {
    vga_clear();
    int i = 5;

    // 0x0A = Hellgrün, 0x0F = White, 0x0C = Red
    vga_print("JOS Modular Architecture: ONLINE\n", 0x0A);
    vga_print("Module 1: VGA Text Driver Loaded.\n", 0x0F);
    vga_print("System Ready.\n", 0x0C);

    while (1) {
    }
}
