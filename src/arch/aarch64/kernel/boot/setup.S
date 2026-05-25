.extern kernelMain
.extern vector_table
.extern __bss_start   // Provided by linker.ld
.extern __bss_end     // Provided by linker.ld

.section ".text.boot"
.global _setup_c_env

_setup_c_env:
    /* ====================================================================
     * 3. ZERO THE BSS SECTION
     * Global/static variables in C that are uninitialized MUST be zero.
     * The bootloader does not do this for us. We loop from __bss_start
     * to __bss_end, writing 64 bits of zero (xzr) at a time.
     * ==================================================================== */
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    sub x2, x1, x0            // Calculate total size of BSS
    cbz x2, skip_bss          // If size is 0, skip entirely

zero_bss_loop:  
    str xzr, [x0], #8         // Store zero register (xzr) into address x0, then x0 += 8
    sub x2, x2, #8            // Decrement remaining size by 8 bytes
    cbnz x2, zero_bss_loop    // If size != 0, loop back

skip_bss:
    /* ====================================================================
     * 4. LOAD THE EXCEPTION VECTOR TABLE
     * We must tell the CPU where our interrupt handlers live before we 
     * execute C code, or an early interrupt will crash the system.
     * ==================================================================== */
    ldr x1, =vector_table
    msr vbar_el1, x1          // Vector Base Address Register for Exception Level 1
    isb

    /* ====================================================================
     * 5. SET THE STACK POINTER
     * C functions push and pop to the stack. sp must be 16-byte aligned.
     * Hardcoding 0x41000000 is dangerous unless your linker guarantees
     * this memory is free and mapped.
     * ==================================================================== */
    ldr x30, =0x41000000
    mov sp, x30

    /* ====================================================================
     * 6. BRANCH TO C KERNEL
     * bl (branch with link) jumps to kernelMain and saves the return
     * address in x30. The kernel should never return, but if it does, 
     * it will fall through to the hang loop.
     * ==================================================================== */
    mov x0, x20
    bl kernelMain

    wfe
    b skip_bss                // Safety catch
