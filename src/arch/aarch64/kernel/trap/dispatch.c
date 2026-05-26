#include "trap.h"
#include "drivers/gic/gic.h"
#include "drivers/uart/uart.h"
#include <stdint.h>

void printHex32(uint32_t val) {
    uartPrintf("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint32_t nibble = (val >> i) & 0xF;
        uartPrintf("%c", nibble > 9 ? 'A' + (nibble - 10) : '0' + nibble);
    }
    uartPrintf("\n");
}

void exceptionDispatch(trap_frame_t *frame) {
    uint32_t ec = (frame->esr >> 26) & 0x3F;

    if (ec == 0x15) {
        handleSynchronousException(frame);
        return;
    }

    uint32_t irq_id = gicReadIAR();
    if (irq_id != 1023) {
        handleHardwareInterrupt(irq_id);
        return;
    }

    kernelPanic(frame, ec);
}