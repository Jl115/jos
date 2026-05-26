# JOS — Architecture Map

## Directory Tree

```
jos/
├── Makefile                          # Kernel cross-compile build
├── src/
│   ├── main.c                        # kernelMain(), initDriverStates, enableCpuIrq
│   ├── linker.ld                     # QEMU virt: RAM at 0x40000000
│   ├── arch/aarch64/
│   │   ├── include/trap.h            # trap_frame_t, handler + dispatch declarations
│   │   └── kernel/
│   │       ├── boot/entry.S          # _entry, boot init
│   │       ├── boot/exeption.S        # Exception handler stubs
│   │       ├── boot/setup.S          # EL and stack setup
│   │       ├── boot/vectors.S         # Exception Vector Table
│   │       └── trap/
│   │           ├── dispatch.c        # exceptionDispatch, printHex32
│   │           ├── irq.c             # handleHardwareInterrupt
│   │           ├── panic.c            # kernelPanic
│   │           └── syscall.c          # handleSynchronousException (SVC path)
│   ├── drivers/
│   │   ├── fdt/
│   │   │   ├── fdt.c                 # parseMemoryFromFdt, dumpFdt, fdt32TOCpu, fdtStrcmp
│   │   │   └── fdt.h                 # fdtHeader struct, FDT token defines, FDT_ALIGN
│   │   ├── gic/
│   │   │   ├── gic.c                 # initGIC, GIC distributor/CPU interface init
│   │   │   ├── gic.h                 # initGIC, gicReadIAR, gicWriteEoir
│   │   │   ├── registry.c            # GIC register access functions (legacy)
│   │   │   └── registry.h            # gicdT/giccT structs, GICD/GICC macros, TEST_BUILD mock
│   │   ├── timer/
│   │   │   ├── timer.c               # readCntfrq, setVirtualTimer, timerHandleTick
│   │   │   └── timer.h
│   │   └── uart/
│   │       ├── handlers.c            # uartPrintf format handler dispatch table
│   │       ├── handlers.h
│   │       ├── uart.c                # PL011 UART init and putc/print
│   │       ├── uart.h
│   │       └── uart_registry.h       # UartRegisters struct, UART0 macro, TEST_BUILD mock
│   └── mm/
│       ├── pfa.c                     # kallocFrame, kfreeFrame, pfaInit (bitmap allocator)
│       └── pfa.h                     # pageframeT, PFA defines/functions
├── tests/
│   ├── Makefile                      # Host-side test build
│   ├── main.c                        # test_main(), register_all_phases
│   ├── phase_registry.c              # RUN_PHASE() for each test phase
│   ├── include/test.h                # PHASE/TEST/ASSERT macros, test_ctx_t
│   ├── mocks/
│   │   ├── mock_gic.c                # volatile gicdT mock_gicd, giccT mock_gicc
│   │   ├── mock_main.c               # endkernel symbol, mockPfaSetup, PFA bitmap storage
│   │   └── mock_uart.c               # uartPutc/Print/Printf stubs, mockUartReset/GetBuffer
│   ├── unit/
│   │   ├── p1-pfa/
│   │   │   ├── test_evt_dispatch.c   # ESR EC field dispatch + trap frame layout tests
│   │   │   ├── test_fdt_pfa.c        # FDT parser + PFA bitmap allocator + real kernel fdt32TOCpu/fdtStrcmp
│   │   │   └── test_trap_dispatch.c  # exceptionDispatch integration with real kernel code + mock GIC
│   │   ├── p2-gic-timers/test_gic_timer.c # GIC register + timer tick + handleHardwareInterrupt tests
│   │   ├── p3-fdt-pfa/
│   │   │   └── test_pfa_integration.c # Real kallocFrame/kfreeFrame/pfaInit against RAM bitmap
│   │   ├── p4-mmu/test_mmu_bitfields.c # MMU descriptor bitfield + MAIR/TCR/VA index tests
│   │   ├── p5-heap/test_heap.c       # Segregated freelist allocator tests
│   │   ├── p6-scheduler/test_scheduler.c # PCB + round-robin scheduler + lifecycle tests
│   │   └── p7-syscall/test_syscall.c  # Syscall dispatch + SPSR/privilege simulation tests
│   └── build/                        # (gitignored) compiled test_runner binary
├── docs/
│   ├── phasePlan.md                  # Phase overview document
│   └── phases/                       # Detailed per-phase PLAN.md files
└── .clang-format, .clang-tidy
```

## Data Flow

```
QEMU boot → entry.S → bss clear → stack init → kernelMain(dtb_ptr)
    │
    ├─ initDriverStates() → uartPrintfInit()
    ├─ parseMemoryFromFdt() → pfaInit() → bitmap PFA
    ├─ initGIC() → GICD/GICC register writes
    ├─ readCntfrq() / setVirtualTimer() → 1ms tick
    ├─ enableCpuIrq() → DAIF clear
    └─ main loop → WFI
```

## Exception Flow

```
Exception → vectors.S → handler stub → C dispatch
    ├─ Synchronous → exceptionDispatch() → handleSynchronousException() / kernelPanic()
    ├─ IRQ → exceptionDispatch() → gicReadIAR() → handleHardwareInterrupt() → gicWriteEoir()
    ├─ FIQ → kernelPanic()
    └─ SError → kernelPanic()
```

## Test Build Architecture

```
tests/main.c + phase_registry.c ──┐
tests/mocks/*.c                   ├─ zig cc (native host) ─→ build/test_runner
tests/unit/**/*.c                 │
../src/drivers/**/*.c (selected)  │
../src/mm/pfa.c                    │   (TEST_BUILD defined)
../src/drivers/fdt/fdt.c          │
../src/arch/aarch64/kernel/trap/dispatch.c │
                                    │
                                    ├─ AArch64 asm excluded by #ifndef TEST_BUILD
                                    └─ MMIO pointers → RAM-backed mock structs
```

## Test Coverage (176 tests, 9 phases)

| Phase | Tests | Tests Real Kernel Code |
|-------|-------|----------------------|
| p1 EVT | ESR EC dispatch, trap frame layout, DAIF masks, SVC/ISS | trap_frame_t offsets |
| p1 Trap Dispatch | exceptionDispatch() integration | exceptionDispatch, GIC mock |
| p3 FDT/PFA | FDT parsing, fdt32TOCpu, fdtStrcmp, PFA bitmap | fdt32TOCpu, fdtStrcmp |
| p3 PFA Integration | pfaInit, kallocFrame, kfreeFrame against real code | pfaInit, kallocFrame, kfreeFrame |
| p2 GIC/Timers | initGIC, IAR/EOIR, timerHandleTick, readCntfrq | initGIC, gicReadIAR, gicWriteEoir, timerHandleTick |
| p4 MMU | Descriptor construction, VA index extraction, MAIR/TCR | Local algorithmic replicas |
| p5 Heap | Segregated freelist alloc/free, bin index, reuse | Local algorithmic replicas |
| p6 Scheduler | PCB, round-robin, state transitions, lifecycle | Local algorithmic replicas |
| p7 Syscall | Dispatch table, privilege, SPSR/ELR | Local algorithmic replicas |