.extern kernel_main
.extern __bss_start   // Defined in your linker.ld
.extern __bss_end     // Defined in your linker.ld

.global _start
.section .text.boot

_start:
    /* 1. Check CPU ID - Park secondary cores */
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbz x0, master
    b hang

master:
    /* 2. Enable FP/SIMD for Zig/Clang */
    mrs x1, cpacr_el1
    orr x1, x1, #(3 << 20)
    msr cpacr_el1, x1
    isb

    /* 3. Zero out the BSS section */
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    sub x2, x1, x0
    cbz x2, 4f
3:  str xzr, [x0], #8     // Write 64 bits of zero
    sub x2, x2, #8
    cbnz x2, 3b
4:

    /* 4. Set stack pointer */
    ldr x30, =0x41000000
    mov sp, x30

    /* 5. Branch to C kernel */
    bl kernel_main

hang:
    wfe
    b hang
