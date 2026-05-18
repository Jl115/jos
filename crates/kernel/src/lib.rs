#![no_std]

use core::fmt::{self, Write};
use core::panic::PanicInfo;
use spin::Mutex;

const UART0: *mut u32 = 0x09000000 as *mut u32;

struct Uart;

impl Write for Uart {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for byte in s.bytes() {
            if byte == b'\n' {
                unsafe {
                    core::ptr::write_volatile(UART0, b'\r' as u32);
                }
            }
            unsafe {
                core::ptr::write_volatile(UART0, byte as u32);
            }
        }
        Ok(())
    }
}

static WRITER: Mutex<Uart> = Mutex::new(Uart);

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

pub fn start() -> ! {
    println!("----------------------------------");
    println!("Kernel-Crate loaded!");
    println!("Loaded kernel mod.");
    println!("----------------------------------");

    loop {
        core::hint::spin_loop();
    }
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("\n[KERNEL PANIC] {}", info);
    loop {
        core::hint::spin_loop();
    }
}
