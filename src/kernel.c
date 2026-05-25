#include "arch/aarch64/include/trap.h"
#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"
#include "drivers/uart/uart.h"
#include <stdint.h>

static void init_driver_states(void) {
    uart_printf_init();
}

static void enable_cpu_irq(void) {
    __asm__ volatile("msr daifclr, #2");
}

void exception_dispatch(trap_frame_t *frame) {
    uint32_t ec = (frame->esr >> 26) & 0x3F;

    if (ec == 0x15) {
        handle_synchronous_exception(frame);
        return;
    }

    uint32_t irq_id = gic_read_iar();
    if (irq_id != 1023) { // 1023 is the Spurious Interrupt ID
        handle_hardware_interrupt(irq_id);
        return;
    }

    kernel_panic(frame, ec);
}

void kernel_main(void) {
    init_driver_states();
    uart_printf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");

    init_gic();

    uint64_t timer_freq = read_cntfrq();
    uint64_t intervall  = timer_freq / 1000; // ms
    set_virtual_timer(intervall);

    enable_cpu_irq();

    while (1) {
        __asm__ volatile("wfi");
    }
}
