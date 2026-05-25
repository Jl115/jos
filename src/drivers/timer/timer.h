#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint64_t systemTicks;
uint64_t                 readCntfrq(void);
void                     timerHandleTick(uint32_t irq_id);
void                     setVirtualTimer(uint64_t intervall);

#endif
