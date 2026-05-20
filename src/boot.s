.extern kernel_main
.global _start
.section .text.boot

_start:
    // 16mb ram start
    ldr x30, =0x41000000
    mov sp, x30
    bl kernel_main

hang:
    wfe
    b hang
