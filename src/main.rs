#![no_std]
#![no_main]

use core::arch::global_asm;

global_asm!(
    ".section .text.boot",
    ".global _start",
    ".balign 4",
    "_start:",
    "adr x30, _start",
    "add x30, x30, #0x100000",
    "mov sp, x30",
    "bl _start_os",
    "1: wfe",
    "b 1b"
);

#[unsafe(no_mangle)]
pub extern "C" fn _start_os() -> ! {
    kernel::start();
}
