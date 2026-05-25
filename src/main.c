#include "drivers/gic/gic.h"
#include "drivers/timer/timer.h"
#include "drivers/uart/uart.h"
#include "trap.h"
#include <stddef.h>
#include <stdint.h>

typedef uint64_t pageframeT;

#define PAGE_SIZE 0x1000
#define FREE      0
#define USED      1
#define ERROR     ((pageframeT) - 1)

// These must be populated dynamically by your Device Tree parser
pageframeT startframe = 0;
uint64_t   npages     = 0;
uint64_t  *bitmap     = NULL;

#define BITS_PER_WORD 64

extern uint32_t          endkernel;
extern volatile uint64_t systemTicks; // Assumed defined in timer.c

static uint64_t nextFreeIdx = 0;

#define FDT_MAGIC      0xd00dfeed
#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE   0x2
#define FDT_PROP       0x3
#define FDT_NOP        0x4
#define FDT_END        0x9

// Byte-swap to handle FDT's Big-Endian format on Little-Endian AArch64
static inline uint32_t fdt32TOCpu(uint32_t val) {
    return __builtin_bswap32(val);
}

void printHex32(uint32_t val) {
    uartPrintf("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint32_t nibble = (val >> i) & 0xF;
        uartPrintf("%c", nibble > 9 ? 'A' + (nibble - 10) : '0' + nibble);
    }
    uartPrintf("\n");
}

// FDT elements are always padded to 4-byte boundaries
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
void dumpFdt(uint64_t dtbPtr) {
    struct fdtHeader *header = (struct fdtHeader *)dtbPtr;

    if (fdt32TOCpu(header->magic) != FDT_MAGIC) {
        uartPrintf("ERROR: Invalid FDT Magic!\n");
        return;
    }

    // Pointers to the structure block and strings block
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

            // Advance pointer past the string and null terminator, then align
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

            // Advance pointer past property data, then align
            ptr = FDT_ALIGN((char *)ptr + len);
        } else if (token == FDT_END) {
            break;
        }
        // FDT_NOP (0x4) is ignored entirely, loop continues
    }
}

pageframeT kallocFrame(void) {
    uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
    uint64_t startWord  = nextFreeIdx / BITS_PER_WORD;

    // Scan forward from the last known free index
    for (uint64_t i = startWord; i < totalWords; i++) {
        if (bitmap[i] != ~0ULL) { // ~0ULL means all bits are 1 (all pages used)
            uint64_t bit       = __builtin_ctzll(~bitmap[i]); // Find index of first 0 bit
            uint64_t pageIndex = (i * BITS_PER_WORD) + bit;

            if (pageIndex >= npages) {
                break; // Hit the padding bits at the end of the last word
            }

            bitmap[i] |= (1ULL << bit); // Set bit to 1 (USED)
            nextFreeIdx = pageIndex + 1;
            return startframe + (pageIndex * PAGE_SIZE);
        }
    }

    // Fallback: Scan from the beginning if we reached the end
    for (uint64_t i = 0; i < startWord; i++) {
        if (bitmap[i] != ~0ULL) {
            uint64_t bit       = __builtin_ctzll(~bitmap[i]);
            uint64_t pageIndex = (i * BITS_PER_WORD) + bit;

            bitmap[i] |= (1ULL << bit);
            nextFreeIdx = pageIndex + 1;
            return startframe + (pageIndex * PAGE_SIZE);
        }
    }

    return ERROR;
}

void kfreeFrame(pageframeT pa) {
    if (pa < startframe)
        return;

    uint64_t index = (pa - startframe) / PAGE_SIZE;

    if (index < npages) {
        uint64_t wordIndex = index / BITS_PER_WORD;
        uint64_t bitOffset = index % BITS_PER_WORD;

        bitmap[wordIndex] &= ~(1ULL << bitOffset); // Set bit to 0 (FREE)

        if (index < nextFreeIdx) {
            nextFreeIdx = index;
        }
    }
}

static void initDriverStates(void) {
    uartPrintfInit();
}

static void enableCpuIrq(void) {
#ifndef TEST_BUILD
    __asm__ volatile("msr daifclr, #2");
#endif
}

void exceptionDispatch(trap_frame_t *frame) {
    uint32_t ec = (frame->esr >> 26) & 0x3F;

    if (ec == 0x15) {
        handleSynchronousException(frame);
        return;
    }

    uint32_t irq_id = gicReadIAR();
    if (irq_id != 1023) {
        handleHardwareInterrupt(irq_id);
        return;
    }

    kernelPanic(frame, ec);
}

// Compare strings without requiring <string.h>
static int fdtStrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// QEMU virt AArch64 uses 64-bit base and size (2 cells each)
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
            // Check if node is "memory" or starts with "memory@"
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
                // Extract Big-Endian 64-bit base and size securely (avoiding alignment faults)
                uint64_t base = ((uint64_t)fdt32TOCpu(ptr[0]) << 32) | fdt32TOCpu(ptr[1]);
                uint64_t size = ((uint64_t)fdt32TOCpu(ptr[2]) << 32) | fdt32TOCpu(ptr[3]);

                startframe = base;
                npages     = size / PAGE_SIZE;

                // Place bitmap in physical memory immediately after the kernel .bss
                // Align endkernel to an 8-byte boundary
                uint64_t kernelEndAddr = (uint64_t)&endkernel;
                kernelEndAddr          = (kernelEndAddr + 7) & ~7ULL;
                bitmap                 = (uint64_t *)kernelEndAddr;

                // Zero out the bitmap (mark all as FREE for now)
                uint64_t totalWords = (npages + BITS_PER_WORD - 1) / BITS_PER_WORD;
                for (uint64_t i = 0; i < totalWords; i++) {
                    bitmap[i] = 0; // 0 = FREE
                }

                // 1. Calculate where the bitmap array ends in physical memory
                uint64_t bitmapSizeBytes = totalWords * sizeof(uint64_t);
                uint64_t pfaEndAddr      = (uint64_t)bitmap + bitmapSizeBytes;

                // 2. Calculate where the FDT blob ends in physical memory
                uint64_t fdtEndAddr = dtbPtr + fdt32TOCpu(header->totalsize);

                // 3. Find the absolute highest address we must protect
                uint64_t reservedEndAddr = (pfaEndAddr > fdtEndAddr) ? pfaEndAddr : fdtEndAddr;

                // 4. Calculate how many pages from the start of RAM this consumes
                uint64_t reservedPages = (reservedEndAddr - startframe + PAGE_SIZE - 1) / PAGE_SIZE;

                // 5. Mark these pages as USED (1) in the bitmap so kallocFrame ignores them
                for (uint64_t i = 0; i < reservedPages; i++) {
                    uint64_t wordIndex = i / BITS_PER_WORD;
                    uint64_t bitOffset = i % BITS_PER_WORD;
                    bitmap[wordIndex] |= (1ULL << bitOffset);
                }

                // 6. Update the next free index to start searching AFTER the kernel
                nextFreeIdx = reservedPages;
                uartPrintf("PFA: Protected %d pages (Kernel, FDT, Bitmap)\n", reservedPages);

                // 7. Parse the FDT Memory Reservation Block
                // The reservation map is guaranteed by spec to be 8-byte aligned.
                uint64_t *rsvmap = (uint64_t *)(dtbPtr + fdt32TOCpu(header->offMemRsvmap));
                while (1) {
                    uint64_t rsvBase = __builtin_bswap64(*rsvmap++);
                    uint64_t rsvSize = __builtin_bswap64(*rsvmap++);

                    if (rsvBase == 0 && rsvSize == 0) {
                        break; // Null-terminated list
                    }

                    // Only reserve if it falls within our managed RAM
                    if (rsvBase >= startframe && rsvBase < (startframe + size)) {
                        uint64_t startPage = (rsvBase - startframe) / PAGE_SIZE;
                        uint64_t endPage =
                            ((rsvBase + rsvSize) - startframe + PAGE_SIZE - 1) / PAGE_SIZE;

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
                break; // Found memory, stop parsing
            }
            ptr = FDT_ALIGN((char *)ptr + len);
        } else if (token == FDT_END) {
            break;
        }
    }
}

// dtb_ptr is passed in x0 by standard AArch64 bootloaders
void kernelMain(uint64_t dtb_ptr) {
    initDriverStates();
    uartPrintf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");

    // Initialize Memory System
    parseMemoryFromFdt(dtb_ptr);

    if (npages == 0) {
        uartPrintf("PANIC: Failed to find /memory node in FDT.\n");
        while (1) {
        }
    }

    // --- PFA Sanity Test ---
    uartPrintf("\n--- PFA Sanity Test ---\n");
    pageframeT p1 = kallocFrame();
    pageframeT p2 = kallocFrame();
    uartPrintf("Allocated P1: 0x%016llx\n", p1);
    uartPrintf("Allocated P2: 0x%016llx\n", p2);

    if (p1 == ERROR || p2 == ERROR || p1 == p2) {
        uartPrintf("PANIC: Allocator is broken.\n");
        while (1)
            ;
    }

    kfreeFrame(p1);
    pageframeT p3 = kallocFrame();
    uartPrintf("Freed P1, Allocated P3: 0x%016llx\n", p3);

    if (p1 == p3) {
        uartPrintf("PFA Test: PASS (Frame Reused)\n\n");
    } else {
        uartPrintf("PFA Test: FAIL (Frame Not Reused)\n\n");
        while (1)
            ;
    }
    // -----------------------

    dumpFdt(dtb_ptr); // Optional: Keep or remove

    initGIC();
    uint64_t timer_freq = readCntfrq();
    setVirtualTimer(timer_freq / 1000);

    enableCpuIrq();

    uint64_t last_tick = 0;

    while (1) {
        if (systemTicks != last_tick) {
            // uart_printf("Tick: %d\n", (uint32_t)system_ticks);
            last_tick = systemTicks;
        }
        __asm__ volatile("wfi");
    }
}
