#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint64_t system_ticks;
uint64_t                 read_cntfrq(void);
void                     timer_handle_tick(uint32_t irq_id);
void                     set_virtual_timer(uint64_t intervall);

#endif
