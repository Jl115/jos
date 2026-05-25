.global vector_table

.extern sync_handler
.extern irq_handler
.extern fiq_handler
.extern serror_handler

/* =======================================================================
 * EXCEPTION VECTOR TABLE
 * The ARM64 vector table requires strict alignment. The table itself
 * must be aligned to 2048 bytes (2^11), and each entry must be exactly 
 * 128 bytes (2^7) apart.
 * ======================================================================= */

.section .vectors, "ax"
.align 11                     // Align table to 2048 bytes

vector_table:
    /* Set 1: Current EL with SP0 (Rarely used, EL1 using EL0 stack) */
    .align 7; b sync_handler  // Align each entry to 128 bytes
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 2: Current EL with SPx (Standard kernel execution at EL1) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 3: Lower EL using AArch64 (64-bit User Space executing at EL0) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler

    /* Set 4: Lower EL using AArch32 (32-bit User Space - Legacy) */
    .align 7; b sync_handler
    .align 7; b irq_handler
    .align 7; b fiq_handler
    .align 7; b serror_handler
