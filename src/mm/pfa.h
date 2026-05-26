#ifndef PFA_H
#define PFA_H

#include <stdint.h>

typedef uint64_t pageframeT;

#define PAGE_SIZE 0x1000
#define FREE      0
#define USED      1
#define ERROR     ((pageframeT) - 1)
#define BITS_PER_WORD 64

extern pageframeT startframe;
extern uint64_t   npages;
extern uint64_t  *bitmap;
extern uint64_t   nextFreeIdx;

void        pfaInit(uint64_t base, uint64_t size, uint64_t reservedStart, uint64_t reservedEnd);
pageframeT  kallocFrame(void);
void        kfreeFrame(pageframeT pa);

#endif