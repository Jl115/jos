.extern _setup_c_env

.section ".text.boot"
.global _start

_start:
    /* ====================================================================
     * 1. PARK SECONDARY CORES
     * The Multiprocessor Affinity Register (mpidr_el1) holds the core ID.
     * We mask out all but the lowest 8 bits. If the result is 0 (Core 0),
     * we continue. If it is 1, 2, or 3, we trap them in an infinite loop.
     * ==================================================================== */
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbz x0, master_core
    b hang

master_core:
    /* ====================================================================
     * 2. ENABLE FLOATING POINT / SIMD
     * Modern compilers (Clang/Zig) will aggressively emit SIMD instructions 
     * even for basic C code. If the FPU is off, the kernel will crash 
     * immediately. We write to the Coprocessor Access Control Register 
     * (cpacr_el1) to unlock it.
     * ==================================================================== */
    mrs x1, cpacr_el1
    orr x1, x1, #(3 << 20)    // Set bits 20 and 21 to enable FP/SIMD at EL1/EL0
    msr cpacr_el1, x1
    isb                       // Instruction Synchronization Barrier: force CPU to apply changes now

    /* Jump to the next phase of boot */
    b _setup_c_env

hang:
    wfe                       // Wait For Event: puts the core to sleep to save power
    b hang
