#include <stdint.h>

#define GICD_BASE 0x08000000 // Distributor Base
#define GICC_BASE 0x08010000 // CPU Interface Base
#define TIMER_IRQ 27         // Virtual Timer (PPI 11)

// GIC Distributor (GICD) Register Map
typedef struct {
    volatile uint32_t CTLR;            // 0x000
    volatile uint32_t _reserved0[63];  // 0x004 - 0x0FC (63 * 4 = 252 bytes)
    volatile uint32_t ISENABLER[32];   // 0x100 - 0x17C (32 * 4 = 128 bytes)
    volatile uint32_t _reserved1[416]; // 0x180 - 0x7FC (416 * 4 = 1664 bytes)
    volatile uint8_t  ITARGETSR[1024]; // 0x800 - 0xBFC (Byte-accessible array)
} gicdT;

// GIC CPU Interface (GICC) Register Map
typedef struct {
    volatile uint32_t CTLR;       // 0x000
    volatile uint32_t PMR;        // 0x004
    volatile uint32_t _reserved0; // 0x008 (Binary Point Register, unused here)
    volatile uint32_t IAR;        // 0x00C
    volatile uint32_t EOIR;       // 0x010
} giccT;

// Cast the base addresses to the struct pointers
#define GICD ((gicd_t *)GICD_BASE)
#define GICC ((gicc_t *)GICC_BASE)
