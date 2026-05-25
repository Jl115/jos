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

void handleSynchronousException(trap_frame_t *frame);
void handleHardwareInterrupt(uint32_t irq_id);
void kernelPanic(trap_frame_t *frame, uint32_t ec);

#endif
