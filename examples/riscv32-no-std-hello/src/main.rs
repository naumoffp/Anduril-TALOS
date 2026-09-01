#![no_std]
#![no_main]

use core::arch::global_asm;
use core::panic::PanicInfo;

// QEMU "virt" machine: NS16550A UART, transmit register at offset 0.
const UART0: *mut u8 = 0x1000_0000 as *mut u8;

// Entry point: set up the stack pointer, then jump into Rust.
global_asm!(
    ".section .text.init",
    ".global _start",          // small assembly stub that sets the stackpointer and calls rust_main
    "_start:",
    "    la sp, _stack_top",
    "    call rust_main",     // writes the string (byte at a time) to UART then spins
    "1:  j 1b",
);

fn uart_write(s: &str) {
    for b in s.bytes() {
        unsafe { UART0.write_volatile(b) };
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_main() -> ! {
    uart_write("Hello from Ferrocene Rust on RISC-V!\n");
    loop {
        core::hint::spin_loop();
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    uart_write("panic!\n");  // handler also prints "panic!"
    loop {
        core::hint::spin_loop();
    }
}
