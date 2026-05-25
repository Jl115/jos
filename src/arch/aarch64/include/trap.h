#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

typedef struct {
    uint64_t x[30];
    uint64_t lr;
    uint64_t elr;
    uint64_t spsr;
    uint64_t esr;
} trap_frame_t;

void handle_synchronous_exception(trap_frame_t *frame);
void handle_hardware_interrupt(uint32_t irq_id);
void kernel_panic(trap_frame_t *frame, uint32_t ec);

#endif
