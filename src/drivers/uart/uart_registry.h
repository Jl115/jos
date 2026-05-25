#ifndef UART_REGS_H
#define UART_REGS_H

#include <stdint.h>

/* * The exact memory layout of the ARM PL011 UART controller.
 * The offsets between the registers must strictly match the
 * hardware specification.
 */
typedef struct {
    volatile uint32_t DATA_REGISTER;              // 0x00
    volatile uint32_t RECIEVE_ERROR_CLEAR_STATUS; // 0x04
    uint32_t          _reserved[4];  // 0x08 to 0x14
    volatile uint32_t FLAG_REGISTER; // 0x18
    uint32_t          _reserved2;    // 0x1C
    volatile uint32_t INTEGER_BAUD_RATE_COUNTER_REGISTER; // 0x20
    volatile uint32_t INTEGER_BAUD_RATE_REGISTER;      // 0x24
} UartRegisters;

#define UART0_BASE 0x09000000

#ifdef TEST_BUILD
extern volatile UartRegisters mock_uart0;
#define UART0 ((UartRegisters *)&mock_uart0)
#else
#define UART0 ((UartRegisters *)UART0_BASE)
#endif

#endif
