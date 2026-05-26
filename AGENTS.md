# JOS — Project Coding Conventions

## Build Commands

- `make run` — Build and boot kernel in QEMU
- `make clean` — Remove build artifacts
- `cd tests && make test` — Compile and run unit tests
- `cd tests && make test-v` — Verbose test output
- `cd tests && make clean` — Remove test build artifacts

## Compiler & Toolchain

- Kernel: `zig cc -target aarch64-freestanding-none`
- Tests: `zig cc` (native host, no target flag)
- C standard: C99 (`-std=c99`)
- Format specifiers: Always use `<inttypes.h>` macros (`PRId64`, `PRIx64`, `PRIu64`) for `int64_t`/`uint64_t` — never `%ld`/`%lx` (breaks on macOS where `long` != `int64_t`)
- Binary literals (`0b...`) are C23 — use hex (`0x0`, `0x1`, `0x2`, `0x3`) for C99 compliance

## Code Style

- Clang-format and clang-tidy configs exist in `.clang-format` and `.clang-tidy`
- No comments unless explicitly requested
- Kernel code is freestanding (`-ffreestanding`, no libc)
- Test code uses hosted C (`stdio.h`, `stdlib.h`, `string.h`, `inttypes.h`)

## Test Build Macro

- `TEST_BUILD` is defined when compiling tests
- All AArch64 inline assembly must be guarded with `#ifndef TEST_BUILD`
- MMIO register pointers use mock RAM-backed structs under `TEST_BUILD` (see `registry.h`, `uart_registry.h`)
- Test files include `"test.h"` — never `".test.h"` or `<test.h>`

## Test Framework

- Use `PHASE("name") { ... }` to group tests by kernel phase
- Use `TEST("desc") { ... }` for individual assertions
- Register new phases in `tests/phase_registry.c` using `RUN_PHASE(name)`
- Mock files go in `tests/mocks/`
- Unit test files go in `tests/unit/pN-<phase-name>/`

## File Naming

- Kernel source: `src/drivers/<device>/<device>.c`, `src/arch/aarch64/kernel/<subsystem>/<file>.c`
- Tests: `tests/unit/pN-<phase-name>/test_<topic>.c`
- Header guards: `GUARD_STYLE_H` (e.g., `REGISTRY_H`, `TRAP_H`)

## Git

- `tests/build/` is gitignored (compiled test runner)
- `build/` is gitignored (compiled kernel objects)
- Never commit binaries