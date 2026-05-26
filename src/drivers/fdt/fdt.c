#include "fdt.h"
#include "drivers/uart/uart.h"
#include "mm/pfa.h"
#include <stddef.h>
#include <stdint.h>

uint32_t fdt32TOCpu(uint32_t val) {
    return __builtin_bswap32(val);
}

int fdtStrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void dumpFdt(uint64_t dtbPtr) {
    struct fdtHeader *header = (struct fdtHeader *)dtbPtr;

    if (fdt32TOCpu(header->magic) != FDT_MAGIC) {
        uartPrintf("ERROR: Invalid FDT Magic!\n");
        return;
    }

    uint32_t *ptr     = (uint32_t *)(dtbPtr + fdt32TOCpu(header->offDTStruct));
    char     *strings = (char *)(dtbPtr + fdt32TOCpu(header->offDTStrings));

    int depth = 0;

    while (1) {
        uint32_t token = fdt32TOCpu(*ptr++);

        if (token == FDT_BEGIN_NODE) {
            char *name = (char *)ptr;
            for (int i = 0; i < depth; i++)
                uartPrintf("  ");
            uartPrintf("%s {\n", name[0] ? name : "/");
            depth++;

            int len = 0;
            while (name[len])
                len++;
            ptr = FDT_ALIGN(name + len + 1);
        } else if (token == FDT_END_NODE) {
            depth--;
            for (int i = 0; i < depth; i++)
                uartPrintf("  ");
            uartPrintf("}\n");
        } else if (token == FDT_PROP) {
            uint32_t len     = fdt32TOCpu(*ptr++);
            uint32_t nameoff = fdt32TOCpu(*ptr++);

            for (int i = 0; i < depth; i++)
                uartPrintf("  ");
            uartPrintf("%s (size: %d)\n", strings + nameoff, len);

            ptr = FDT_ALIGN((char *)ptr + len);
        } else if (token == FDT_END) {
            break;
        }
    }
}

void parseMemoryFromFdt(uint64_t dtbPtr) {
    struct fdtHeader *header = (struct fdtHeader *)dtbPtr;

    if (fdt32TOCpu(header->magic) != FDT_MAGIC) {
        uartPrintf("PANIC: Invalid FDT Magic in parseMemoryFromFdt\n");
        while (1)
            ;
    }

    uint32_t *ptr     = (uint32_t *)(dtbPtr + fdt32TOCpu(header->offDTStruct));
    char     *strings = (char *)(dtbPtr + fdt32TOCpu(header->offDTStrings));

    int inMemoryNode = 0;

    while (1) {
        uint32_t token = fdt32TOCpu(*ptr++);

        if (token == FDT_BEGIN_NODE) {
            char *name = (char *)ptr;
            if (fdtStrcmp(name, "memory") == 0 ||
                (name[0] == 'm' && name[1] == 'e' && name[2] == 'm' && name[3] == 'o' &&
                 name[4] == 'r' && name[5] == 'y' && name[6] == '@')) {
                inMemoryNode = 1;
            } else {
                inMemoryNode = 0;
            }

            int len = 0;
            while (name[len])
                len++;
            ptr = FDT_ALIGN(name + len + 1);

        } else if (token == FDT_END_NODE) {
            inMemoryNode = 0;
        } else if (token == FDT_PROP) {
            uint32_t len      = fdt32TOCpu(*ptr++);
            uint32_t nameoff  = fdt32TOCpu(*ptr++);
            char    *propName = strings + nameoff;

            if (inMemoryNode && fdtStrcmp(propName, "reg") == 0) {
                uint64_t base = ((uint64_t)fdt32TOCpu(ptr[0]) << 32) | fdt32TOCpu(ptr[1]);
                uint64_t size = ((uint64_t)fdt32TOCpu(ptr[2]) << 32) | fdt32TOCpu(ptr[3]);

                extern uint32_t endkernel;
                uint64_t kernelEndAddr = (uint64_t)&endkernel;
                kernelEndAddr          = (kernelEndAddr + 7) & ~7ULL;
                bitmap                 = (uint64_t *)kernelEndAddr;

                uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
                uint64_t bitmapSizeBytes = totalWords * sizeof(uint64_t);
                uint64_t pfaEndAddr      = (uint64_t)bitmap + bitmapSizeBytes;
                uint64_t fdtEndAddr      = dtbPtr + fdt32TOCpu(header->totalsize);
                uint64_t reservedEndAddr = (pfaEndAddr > fdtEndAddr) ? pfaEndAddr : fdtEndAddr;

                pfaInit(base, size, startframe, reservedEndAddr);

                uint64_t *rsvmap = (uint64_t *)(dtbPtr + fdt32TOCpu(header->offMemRsvmap));
                while (1) {
                    uint64_t rsvBase = __builtin_bswap64(*rsvmap++);
                    uint64_t rsvSize = __builtin_bswap64(*rsvmap++);

                    if (rsvBase == 0 && rsvSize == 0) {
                        break;
                    }

                    if (rsvBase >= startframe && rsvBase < (startframe + size)) {
                        uint64_t startPage = (rsvBase - startframe) / PAGE_SIZE;
                        uint64_t endPage   = ((rsvBase + rsvSize) - startframe + PAGE_SIZE - 1) / PAGE_SIZE;

                        for (uint64_t i = startPage; i < endPage && i < npages; i++) {
                            uint64_t wordIndex = i / BITS_PER_WORD;
                            uint64_t bitOffset = i % BITS_PER_WORD;
                            bitmap[wordIndex] |= (1ULL << bitOffset);
                        }
                        uartPrintf("PFA: Reserved FDT Block [0x%016llx - 0x%016llx]\n", rsvBase,
                                   rsvBase + rsvSize);
                    }
                }

                uartPrintf("RAM: Base 0x%016llx, Size 0x%016llx (%d pages)\n", base, size, npages);
                uartPrintf("PFA: Bitmap placed at 0x%016llx\n", (uint64_t)bitmap);
                break;
            }
            ptr = FDT_ALIGN((char *)ptr + len);
        } else if (token == FDT_END) {
            break;
        }
    }
}