ENTRY(_start)

SECTIONS
{
    /* QEMU virt machine RAM starts at 0x40000000 */
    . = 0x40000000;

    .text : ALIGN(4K)
    {
        /* The boot assembly MUST be the very first thing in the binary */
        *(.text.boot)
        *(.text*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data*)
    }

    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
        endkernel = .;
    }
}
