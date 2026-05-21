.extern kernel_main
.global _start
.section .text.boot

_start:
    /* Read Architectural Feature Access Control Register */
    mrs x1, cpacr_el1
    
    /* Enable FP/SIMD (Set bits 20 and 21) */
    orr x1, x1, #(3 << 20) 
    
    /* Write back and flush pipeline */
    msr cpacr_el1, x1
    isb

    /* Set stack pointer to 16MB into RAM (0x41000000) */
    ldr x30, =0x41000000
    mov sp, x30

    /* Branch to C kernel */
    bl kernel_main

hang:
    wfe
    b hang
