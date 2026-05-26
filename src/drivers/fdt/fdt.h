#ifndef FDT_H
#define FDT_H

#include <stdint.h>

#define FDT_MAGIC      0xd00dfeed
#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE   0x2
#define FDT_PROP       0x3
#define FDT_NOP        0x4
#define FDT_END        0x9

#define FDT_ALIGN(p) ((uint32_t *)(((uintptr_t)(p) + 3) & ~3))

struct __attribute__((packed)) fdtHeader {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t offDTStruct;
    uint32_t offDTStrings;
    uint32_t offMemRsvmap;
    uint32_t version;
    uint32_t lastCompVersion;
    uint32_t bootCpuidPhys;
    uint32_t sizeDTStrings;
    uint32_t sizeDTStruct;
};

uint32_t fdt32TOCpu(uint32_t val);
int      fdtStrcmp(const char *s1, const char *s2);
void     dumpFdt(uint64_t dtbPtr);
void     parseMemoryFromFdt(uint64_t dtbPtr);

#endif