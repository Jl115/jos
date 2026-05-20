#ifndef UART_REGS_H
#define UART_REGS_H

typedef unsigned int uint32_t;

#define UART0_BASE 0x09000000

/* * The exact memory layout of the ARM PL011 UART controller.
 * The offsets between the registers must strictly match the
 * hardware specification.
 */
typedef struct {
    volatile uint32_t DATA_REGISTER;             // 0x00 - Data Register
    volatile uint32_t REIECE_ERROR_CLEAR_STATUS; // 0x04 - Receive Status / Error Clear Register
    uint32_t          _reserved[4];  // 0x08 to 0x14 - Reserved memory space (padding/gap in layout)
    volatile uint32_t FLAG_REGISTER; // 0x18 - Flag Register
    uint32_t          _reserved2;    // 0x1C
    volatile uint32_t IRDA_LOW_POWER_COUNTER_REGISTER; // 0x20 - IrDA Low-Power Counter Register
    volatile uint32_t INTEGER_BAUD_RATE_REGISTER;      // 0x24 - Integer Baud Rate Register
} UartRegisters;

// A global pointer mapping directly to the hardware base address
#define UART0 ((UartRegisters *)UART0_BASE)

#endif
