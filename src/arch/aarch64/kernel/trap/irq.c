#include "arch/aarch64/include/trap.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"
#include "drivers/uart/uart.h"

void handleHardwareInterrupt(uint32_t irq_id) {
    if (irq_id == 27) {
        timerHandleTick(irq_id);
        gicWriteEoir(irq_id);
        return;
    }
    uartPrintf("Unhandled IRQ: %d\n", irq_id);
    gicWriteEoir(irq_id);
}
