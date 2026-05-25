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
uint8_t   *frameMap   = NULL;

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
    for (uint64_t i = nextFreeIdx; i < npages; i++) {
        if (frameMap[i] == FREE) {
            frameMap[i] = USED;
            nextFreeIdx = i + 1;
            return startframe + (i * PAGE_SIZE);
        }
    }

    // Fallback scan
    for (uint64_t i = 0; i < nextFreeIdx; i++) {
        if (frameMap[i] == FREE) {
            frameMap[i] = USED;
            nextFreeIdx = i + 1;
            return startframe + (i * PAGE_SIZE);
        }
    }

    return ERROR;
}

void kfreeFrame(pageframeT pa) {
    if (pa < startframe)
        return;

    uint64_t index = (pa - startframe) / PAGE_SIZE;

    if (index < npages) {
        frameMap[index] = FREE;
        if (index < nextFreeIdx) {
            nextFreeIdx = index;
        }
    }
}

static void initDriverStates(void) {
    uartPrintfInit();
}

static void enableCpuIrq(void) {
    __asm__ volatile("msr daifclr, #2");
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

// dtb_ptr is passed in x0 by standard AArch64 bootloaders
void kernelMain(uint64_t dtb_ptr) {
    initDriverStates();
    uartPrintf("\x1b[32mJOS Modular Architecture: ONLINE\n\x1b[0m");
    printHex32(*(uint32_t *)dtb_ptr);
    uartPrintf("Raw memory at dtb_ptr: 0x%08x\n", *(uint32_t *)dtb_ptr);

    // TODO: Implement parse_dtb(dtb_ptr) here to extract memory nodes
    // and initialize startframe, npages, and allocate frame_map.
    // parse_dtb(dtb_ptr);

    uartPrintf("--- FDT DUMP START ---\n");
    dumpFdt(dtb_ptr);
    uartPrintf("--- FDT DUMP END ---\n");

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
