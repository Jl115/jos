#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdint.h>

/* =======================================================================
 * QEMU 'virt' Machine Memory Map (AArch64)
 * ======================================================================= */

#define GICD_BASE 0x08000000 // GICv2 Distributor Base Address
#define GICC_BASE 0x08010000 // GICv2 CPU Interface Base Address

/* =======================================================================
 * Interrupt IDs (PPIs and SPIs)
 * ======================================================================= */

// Virtual Timer Interrupt (PPI 11 -> 16 + 11 = 27)
#define TIMER_IRQ 27

/* =======================================================================
 * GICv2 Distributor (GICD) Register Map
 * Calculated padding ensures exact memory alignment.
 * ======================================================================= */
typedef struct {
    volatile uint32_t CTLR;            // 0x000: Distributor Control Register
    volatile uint32_t _reserved0[63];  // 0x004 - 0x0FC: Reserved (252 bytes)
    volatile uint32_t ISENABLER[32];   // 0x100 - 0x17C: Interrupt Set-Enable Registers
    volatile uint32_t _reserved1[416]; // 0x180 - 0x7FC: Reserved (1664 bytes)
    volatile uint8_t  ITARGETSR[1024]; // 0x800 - 0xBFC: Interrupt Processor Targets Registers
} gicdT;

/* =======================================================================
 * GICv2 CPU Interface (GICC) Register Map
 * ======================================================================= */
typedef struct {
    volatile uint32_t CTLR; // 0x000: CPU Interface Control Register
    volatile uint32_t PMR;  // 0x004: Interrupt Priority Mask Register
    volatile uint32_t BPR;  // 0x008: Binary Point Register
    volatile uint32_t IAR;  // 0x00C: Interrupt Acknowledge Register
    volatile uint32_t EOIR; // 0x010: End of Interrupt Register
} giccT;

/* =======================================================================
 * Hardware Register Pointers
 * Use these macros to interact with the hardware in your driver files.
 * ======================================================================= */

#define GICD ((gicdT *)GICD_BASE)
#define GICC ((giccT *)GICC_BASE)

#endif // REGISTRY_H
