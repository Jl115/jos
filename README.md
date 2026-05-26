# JOS

A bare-metal ARM64 (AArch64) operating system kernel targeting the QEMU `virt` machine. Built from scratch as a learning project through 7 incremental phases — from exception handling to user-space system calls.

## Features

- **ARM64 bare-metal** — runs on QEMU `virt` (Cortex-A53)
- **Exception Vector Table** — catches sync exceptions, IRQs, FIQs, and SErrors
- **GICv2 driver** — interrupt distribution and CPU interface initialization
- **ARM Generic Timer** — 1 ms tick heartbeat
- **FDT parser** — discovers RAM from the QEMU-provided device tree
- **Page Frame Allocator (PFA)** — bitmap-based physical page allocator
- **Host-side unit tests** — runs natively on macOS/Linux with `zig cc`

## Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| [Zig](https://ziglang.org) | 0.11+ | `brew install zig` |
| [QEMU](https://www.qemu.org) | 7+ | `brew install qemu` |
| Make | Any | macOS built-in |

Verify:

```sh
zig version
qemu-system-aarch64 --version
```

## Quick Start

### Build and Run the Kernel

```sh
make run
```

This compiles the kernel with `zig cc -target aarch64-freestanding-none`, links it with `src/linker.ld`, and boots it in QEMU.

### Build Only

```sh
make
```

The ELF is output to `build/jos_kernel8.elf`.

### Build a Raw Disk Image

```sh
make img
```

Outputs `jos.img` (flat binary).

### Clean

```sh
make clean
```

## Running Tests

The test suite compiles and runs **natively on your host machine** (macOS/Linux) using `zig cc` — no QEMU needed.

```sh
cd tests
make test        # compile and run all tests
make test-v      # verbose per-test output
make clean       # remove tests/build/
```

Tests validate kernel logic (ESR decoding, FDT parsing, PFA allocation, GIC state, MMU bitfields, heap, scheduler, syscalls) using mock hardware registers backed by RAM instead of real MMIO.

### Test Framework

The test framework is in `tests/include/test.h` and provides:

| Macro | Purpose |
|-------|---------|
| `PHASE("name") { ... }` | Define a test phase |
| `TEST("desc") { ... }` | Define a test case |
| `ASSERT_TRUE(cond)` | Assert condition is true |
| `ASSERT_FALSE(cond)` | Assert condition is false |
| `ASSERT_EQ_INT(a, b)` | Assert `int64_t` equality |
| `ASSERT_NE_INT(a, b)` | Assert `int64_t` inequality |
| `ASSERT_EQ_UINT(a, b)` | Assert `uint64_t` equality (hex output) |
| `ASSERT_EQ_HEX(a, b)` | Alias for `ASSERT_EQ_UINT` |
| `ASSERT_PTR_NULL(p)` | Assert pointer is NULL |
| `ASSERT_PTR_NOT_NULL(p)` | Assert pointer is not NULL |
| `ASSERT_STRING_EQ(a, b)` | Assert C string equality |
| `ASSERT_MEM_EQ(p1, p2, n)` | Assert memory equality |
| `SKIP("msg")` | Skip the current test |

## Project Structure

```
jos/
├── Makefile                  # Kernel build (zig cc cross-compile)
├── src/
│   ├── main.c                # Kernel entry, FDT parser, PFA, exception dispatch
│   ├── linker.ld             # Linker script (RAM at 0x40000000)
│   ├── arch/aarch64/
│   │   ├── include/trap.h    # Trap frame struct, handler declarations
│   │   └── kernel/
│   │       ├── boot/
│   │       │   ├── entry.S   # Boot entry point
│   │       │   ├── exeption.S
│   │       │   ├── setup.S
│   │       │   └── vectors.S # Exception Vector Table
│   │       └── trap/
│   │           ├── irq.c     # IRQ handler dispatch
│   │           ├── panic.c    # Kernel panic
│   │           └── syscall.c  # System call handler
│   └── drivers/
│       ├── gic/
│       │   ├── gic.c         # GICv2 driver implementation
│       │   ├── gic.h
│       │   ├── registry.c
│       │   └── registry.h    # GIC register map + TEST_BUILD mock pointers
│       ├── timer/
│       │   ├── timer.c       # ARM Generic Timer driver
│       │   └── timer.h
│       └── uart/
│           ├── handlers.c    # UART printf format handler dispatch
│           ├── handlers.h
│           ├── uart.c        # PL011 UART driver
│           ├── uart.h
│           └── uart_registry.h # UART register map + TEST_BUILD mock
├── tests/
│   ├── Makefile              # Test build (zig cc native host)
│   ├── main.c                # Test runner entry point
│   ├── phase_registry.c      # Registers all test phases
│   ├── include/test.h         # Test framework macros and globals
│   ├── mocks/
│   │   ├── mock_gic.c        # RAM-backed GIC register mocks
│   │   └── mock_uart.c       # UART stubs (routes printf to stderr)
│   ├── unit/
│   │   ├── p1-pfa/           # Phase 1: Exception Vector Table
│   │   │   ├── test_evt_dispatch.c
│   │   │   └── test_fdt_pfa.c
│   │   ├── p2-gic-timers/    # Phase 2: GIC & Timers
│   │   │   └── test_gic_timer.c
│   │   ├── p4-mmu/           # Phase 4: MMU & Paging
│   │   │   └── test_mmu_bitfields.c
│   │   ├── p5-heap/          # Phase 5: Kernel Heap
│   │   │   └── test_heap.c
│   │   ├── p6-scheduler/     # Phase 6: Multitasking & Scheduling
│   │   │   └── test_scheduler.c
│   │   └── p7-syscall/       # Phase 7: User Space & System Calls
│   │       └── test_syscall.c
│   └── build/                # Compiled test runner (gitignored)
├── docs/
│   ├── phasePlan.md          # Phase overview
│   └── phases/               # Detailed phase plans
│       ├── README.md
│       ├── phase-01-exception-vector-table/PLAN.md
│       ├── phase-02-gic-and-timers/PLAN.md
│       ├── phase-03-fdt-and-pfa/PLAN.md
│       ├── phase-04-mmu-and-paging/PLAN.md
│       ├── phase-05-kernel-heap/PLAN.md
│       ├── phase-06-multitasking-and-scheduling/PLAN.md
│       └── phase-07-user-space-and-system-calls/PLAN.md
└── .clang-format
└── .clang-tidy
```

## Phase Roadmap

| Phase | Topic | Status |
|-------|-------|--------|
| 1 | Exception Vector Table | Done |
| 2 | GIC & Timers | Done |
| 3 | FDT Parser & Page Frame Allocator | Done |
| 4 | MMU & Paging | In progress |
| 5 | Kernel Heap | Planned |
| 6 | Multitasking & Scheduling | Planned |
| 7 | User Space & System Calls | Planned |

See `docs/phases/` for detailed implementation plans for each phase.

## Build System

The project uses two separate Makefiles:

- **`Makefile`** (root) — Cross-compiles the kernel for `aarch64-freestanding-none` using `zig cc`. Produces `build/jos_kernel8.elf` and `jos.img`.
- **`tests/Makefile`** — Compiles tests natively on the host using `zig cc` (no target triple, defaults to host). Produces `tests/build/test_runner`.

The `TEST_BUILD` macro (`-DTEST_BUILD`) is defined when compiling tests. This substitutes:
- MMIO register pointers → RAM-backed mock structs
- AArch64 inline assembly → host-safe stubs
- UART output → `stderr` via `vfprintf`

## Debugging

### QEMU Monitor

```sh
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel build/jos_kernel8.elf
```

### GDB Remote Debugging

```sh
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel build/jos_kernel8.elf -s -S
# In another terminal:
aarch64-none-elf-gdb build/jos_kernel8.elf -ex "target remote :1234"
```

### QEMU Trace Flags

```sh
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic \
  -kernel build/jos_kernel8.elf -d int     # interrupt traces
```

## Resources

| Resource | Link |
|----------|------|
| ARM Architecture Reference Manual (ARMv8-A) | [DDI 0487](https://developer.arm.com/documentation/ddi0487/latest/) |
| ARM Cortex-A Series Programmer's Guide | [DEN0024](https://developer.arm.com/documentation/den0024/latest/) |
| QEMU `virt` Machine Docs | [qemu.org](https://www.qemu.org/docs/master/system/arm/virt.html) |
| OSDev Wiki | [wiki.osdev.org](https://wiki.osdev.org/Main_Page) |
| xv6 (MIT) | [github.com/mit-pdos/xv6-riscv](https://github.com/mit-pdos/xv6-riscv) |

## License

This project is for educational purposes.
