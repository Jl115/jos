#include "../uart/uart.h"
#include <stdint.h>

uint64_t read_cntfrq(void) {
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    uart_printf("\x1b[32mJOS Frequency is: %uHz\x1b[0m\n", freq);
    return freq;
}
