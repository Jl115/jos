#include "registry.h"
#include <stdint.h>

void initGIC(void) {
    // Disable during setup
    GICD->CTLR = 0;
    GICC->CTLR = 0;

    // Route IRQ to CPU 0
    // Because ITARGETSR is defined as uint8_t, we map 1:1 with the IRQ number
    GICD->ITARGETSR[TIMER_IRQ] = 1;

    // Enable the IRQ
    // Divide by 32 to find the register index, modulo 32 to find the bit
    GICD->ISENABLER[TIMER_IRQ / 32] = (1 << (TIMER_IRQ % 32));

    // Allow all priorities
    GICC->PMR = 0xFFFF;

    // Re-enable
    GICD->CTLR = 1;
    GICC->CTLR = 1;
}

uint32_t gicReadIAR(void) {
    return GICC->IAR;
}

void gicWriteEoir(uint32_t irq) {
    GICC->EOIR = irq;
}
