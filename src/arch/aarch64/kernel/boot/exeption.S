.extern exception_dispatch

.global sync_handler
.global irq_handler
.global fiq_handler
.global serror_handler

/* =======================================================================
 * KERNEL ENTRY MACRO
 * When an interrupt occurs, the CPU jumps here but does not save registers.
 * If we do not save x0-x29, the C handler will overwrite them and corrupt
 * the interrupted program.
 * ======================================================================= */
.macro kernel_entry
    /* Allocate 272 bytes on the stack. ARM64 requires 16-byte stack alignment */
    sub sp, sp, #272
    
    /* Save pairs of registers (stp) to memory offsets */
    stp x0, x1, [sp, #16 * 0]
    stp x2, x3, [sp, #16 * 1]
    stp x4, x5, [sp, #16 * 2]
    stp x6, x7, [sp, #16 * 3]
    stp x8, x9, [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]

    /* Save vital system registers:
     * elr_el1: Exception Link Register (where to return after interrupt)
     * spsr_el1: Saved Program Status Register (CPU flags at time of interrupt)
     * esr_el1: Exception Syndrome Register (WHY the interrupt happened) */
    mrs x21, elr_el1
    mrs x22, spsr_el1
    mrs x23, esr_el1

    stp x30, x21, [sp, #16 * 15] // x30 is the standard Link Register
    stp x22, x23, [sp, #16 * 16]
.endm

/* =======================================================================
 * KERNEL EXIT MACRO
 * Reverses kernel_entry. Restores the exact CPU state right before
 * the interrupt occurred so execution can resume transparently.
 * ======================================================================= */
.macro kernel_exit
    ldp x22, x23, [sp, #16 * 16]
    ldp x30, x21, [sp, #16 * 15]

    msr elr_el1, x21
    msr spsr_el1, x22

    ldp x0, x1, [sp, #16 * 0]
    ldp x2, x3, [sp, #16 * 1]
    ldp x4, x5, [sp, #16 * 2]
    ldp x6, x7, [sp, #16 * 3]
    ldp x8, x9, [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]

    add sp, sp, #272
    
    /* Exception Return: restores PC from ELR and PSTATE from SPSR */
    eret
.endm

/* =======================================================================
 * EXCEPTION HANDLER JUMPS
 * These act as the glue between the vector table branches and the C code.
 * ======================================================================= */

.section .text

sync_handler:
    kernel_entry
    mov x0, sp               // Pass stack pointer (trap frame) to C as arg1
    bl exception_dispatch
    kernel_exit

irq_handler:
    kernel_entry
    mov x0, sp
    bl exception_dispatch
    kernel_exit

fiq_handler:
    kernel_entry
    mov x0, sp
    bl exception_dispatch
    kernel_exit

serror_handler:
    kernel_entry
    mov x0, sp
    bl exception_dispatch
    kernel_exit
