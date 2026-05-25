#ifndef GIC_H
#define GIC_H

#include <stdint.h>

uint32_t gic_read_iar(void);
void     init_gic(void);
void     gic_write_eoir(uint32_t irq);

#endif // GIC_H
