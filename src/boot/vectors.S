.extern exception_dispatch

/* =======================================================================
 * CONTEXT SAVING MACROS
 * ======================================================================= */

.macro kernel_entry
    /* Allocate 272 bytes on the stack for 34 registers (34 * 8 bytes).
       This maintains the mandatory 16-byte stack alignment. */
    sub sp, sp, #272
    
    /* Save General Purpose Registers x0 - x29 */
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

    /* Read system exception registers */
    mrs x21, elr_el1
    mrs x22, spsr_el1
    mrs x23, esr_el1

    /* Save Link Register (x30), ELR, SPSR, and ESR */
    stp x30, x21, [sp, #16 * 15]
    stp x22, x23, [sp, #16 * 16]
.endm

.macro kernel_exit
    /* Restore SPSR and ESR (We pop ESR to keep stack aligned, but we don't write it back to hardware) */
    ldp x22, x23, [sp, #16 * 16]
    /* Restore Link Register (x30) and ELR */
    ldp x30, x21, [sp, #16 * 15]

    /* Write back to hardware system registers. 
       If your C code modified the return address in the struct, it takes effect here. */
    msr elr_el1, x21
    msr spsr_el1, x22

    /* Restore General Purpose Registers x0 - x29 */
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

    /* Deallocate the trap frame from the stack */
    add sp, sp, #272
    
    /* Return to the instruction in ELR_EL1 and restore PSTATE from SPSR_EL1 */
    eret
.endm

/* =======================================================================
 * EXCEPTION VECTOR TABLE
 * ======================================================================= */

.section .vectors, "ax"
.align 11
.global vector_table

vector_table:
    /* Set 1: Current EL with SP0 (Kernel thread using EL0 stack - rarely used) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 2: Current EL with SPx (Kernel thread using EL1 stack - default in early boot) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 3: Lower EL using AArch64 (64-bit User space execution) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 4: Lower EL using AArch32 (32-bit User space execution) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

/* =======================================================================
 * EXCEPTION HANDLERS
 * ======================================================================= */

/* Revert to standard text section for the handler code */
.section .text

sync_handler:
    kernel_entry
    /* Pass the stack pointer (pointing to trap_frame_t) as argument 1 (x0) to C */
    mov x0, sp
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
