# JOS Learning Plans

This directory contains detailed study plans for building **JOS** — a hobby operating system for **ARM64 (AArch64)** targeting QEMU `virt`.

Each phase builds on the previous one. Read them in order. Do not skip ahead until you've implemented and fully understood each phase.

---

## The Roadmap

| Phase                                           | Topic                              | What You'll Learn                                                                                |
| ----------------------------------------------- | ---------------------------------- | ------------------------------------------------------------------------------------------------ |
| [Phase 1](phase-01-exception-vector-table)      | Exception Vector Table             | How ARM64 exceptions work, how to build the EVT, and how to catch CPU faults from C.             |
| [Phase 2](phase-02-gic-and-timers)              | GIC & Timers                       | Hardware interrupts, the ARM Generic Timer, and how to give your OS a heartbeat.                 |
| [Phase 3](phase-03-fdt-and-pfa)                 | Device Tree & Page Frame Allocator | Parse the Flattened Device Tree to discover RAM, then manage it as 4 KiB physical pages.         |
| [Phase 4](phase-04-mmu-and-paging)              | MMU & Paging                       | The hardest phase. Build ARM64 translation tables, configure MAIR/TCR/TTBR, and turn on the MMU. |
| [Phase 5](phase-05-kernel-heap)                 | Kernel Heap                        | A `kmalloc`/`kfree` allocator for variable-sized objects on top of the PFA.                      |
| [Phase 6](phase-06-multitasking-and-scheduling) | Multitasking & Scheduling          | Process Control Blocks, context switches, and a round-robin scheduler.                           |
| [Phase 7](phase-07-user-space-and-system-calls) | User Space & System Calls          | Drop to EL0, isolate user processes, and implement `SVC`-based system calls.                     |

---

## How to Use These Plans

1. **Read** the `PLAN.md` in each phase directory.
2. **Answer** the "Key Questions" at the bottom of each plan. If you can't answer them, re-read the resources.
3. **Study** the linked resources — official ARM manuals, OSDev wiki, Linux kernel source, and tutorials.
4. **Write** your implementation. Use the pseudocode as a guide, but write ALL the code yourself.
5. **Test thoroughly** before moving to the next phase. A bug in Phase 1 will make Phase 4 impossible.

---

## General Resources (Not Phase-Specific)

| Resource                                                     | URL                                                      | Why Read It                                                                                       |
| ------------------------------------------------------------ | -------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| **OSDev Wiki — Main Page**                                   | https://wiki.osdev.org/Main_Page                         | The go-to community wiki for hobby OS development. Read the "Getting Started" and "ARM" sections. |
| **ARM Architecture Reference Manual for ARMv8-A (DDI 0487)** | https://developer.arm.com/documentation/ddi0487/latest/  | The ultimate reference for everything ARM64. Keep it open while coding.                           |
| **ARM Cortex-A Series Programmer's Guide (DEN0024)**         | https://developer.arm.com/documentation/den0024/latest/  | A gentler introduction to ARM64 architecture than the full Reference Manual.                      |
| **QEMU `virt` Machine Documentation**                        | https://www.qemu.org/docs/master/system/arm/virt.html    | Know your target platform: memory layout, devices, and boot behavior.                             |
| **Raspberry Pi OS Tutorial (Sergey Matyukevich)**            | https://github.com/s-matyukevich/raspberry-pi-os         | Step-by-step bare-metal ARM64 OS tutorials. Not QEMU but very close.                              |
| **xv6 (MIT)**                                                | https://github.com/mit-pdos/xv6-riscv                    | A teaching OS for RISC-V. Excellent explanations of OS concepts. Translate assembly mentally.     |
| **Linux Kernel Source**                                      | https://github.com/torvalds/linux/tree/master/arch/arm64 | The most mature ARM64 OS. Read `kernel/entry.S`, `kernel/process.c`, `mm/`, etc.                  |

---

## Development Tips

- **Build incrementally.** Test at every sub-step. Use `uart_printf` as your primary debugging tool.
- **Use QEMU's `-d` flags** for debugging: `-d int` shows interrupts, `-d exec` shows executed instructions, `-d in_asm` shows disassembly.
- **QEMU monitor:** Connect with `-monitor telnet:localhost:1235,server` or `-serial mon:stdio`.
- **GDB stub:** QEMU supports `-s -S` to wait for GDB on port 1234. Use `aarch64-none-elf-gdb`.
- **Don't be afraid to crash.** That's what QEMU is for. If you hit a data abort early, it means your EVT from Phase 1 is working.
