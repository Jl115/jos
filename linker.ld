ENTRY(_start)
SECTIONS {
    . = 0x40000000;
    
    .text : { 
        KEEP(*(.text.boot))  
        *(.text .text.*) 
    }
    
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        __bss_end = .;
    }
}
