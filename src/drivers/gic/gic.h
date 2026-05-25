#ifndef GIC_H
#define GIC_H

#include <stdint.h>

uint32_t gicReadIAR(void);
void     initGIC(void);
void     gicWriteEoir(uint32_t irq);

#endif // GIC_H
