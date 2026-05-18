#![no_std]
#![no_main]

use core::arch::global_asm;
use core::panic::PanicInfo;
use core::fmt::{self, Write};
use spin::Mutex;

// 1. BOOT
global_asm!(
    ".section .text.boot",
    ".global _start",
    ".balign 4",
    "_start:",
    "adr x30, _start",
    "add x30, x30, #0x100000",
    "mov sp, x30",
    "bl rust_main",
    "1: wfe",
    "b 1b"
);

const UART0: *mut u32 = 0x09000000 as *mut u32;

struct Uart;

impl Write for Uart {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for byte in s.bytes() {
            if byte == b'\n' {
                unsafe { core::ptr::write_volatile(UART0, b'\r' as u32); }
            }
            unsafe { core::ptr::write_volatile(UART0, byte as u32); }
        }
        Ok(())
    }
}

// Global Sync
static WRITER: Mutex<Uart> = Mutex::new(Uart);

// Macros
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    WRITER.lock().write_fmt(args).unwrap();
}

// Kernel
#[unsafe(no_mangle)]
pub extern "C" fn rust_main() -> ! {
    println!("----------------------------------");
    println!("System bootet...");
    println!("Willkommen im Bare-Metal AArch64 mit Rust!");
    println!("Formatierung funktioniert: {} + {} = {}", 2, 2, 4);
    println!("UART0 Register liegt bei: {:p}", UART0);
    println!("----------------------------------");

    loop {
        core::hint::spin_loop();
    } 
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("\n[KERNEL PANIC]\n{}", info);
    loop {
        core::hint::spin_loop();
    }
}
