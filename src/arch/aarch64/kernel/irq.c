#include "arch/aarch64/include/trap.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"
#include "drivers/uart/uart.h"

void handle_hardware_interrupt(uint32_t irq_id) {
    if (irq_id == 27) {
        timer_handle_tick(irq_id);
        gic_write_eoir(irq_id);
        return;
    }
    uart_printf("Unhandled IRQ: %d\n", irq_id);
    gic_write_eoir(irq_id);
}
