# ===========================================================================
# JOS Host-Side Unit Test Harness
# ===========================================================================
# Compiles with zig cc on the HOST (not cross-compiling) so tests run
# natively on the development machine.
#
# Usage:
#   make test      -- compile and run all tests
#   make test-v    -- compile and run with verbose per-test output
#   make clean     -- remove build/ directory
# ===========================================================================

# Use ?= so the CI or user can override with `make CC=zig cc`
CC       ?= zig cc
CFLAGS   := -Wall -Wextra -std=c99 -I./include \
            -I../src -I../src/arch/aarch64/include -I../src/drivers \
            -DDEBUG -g -O0 -DTEST_BUILD

# --- Kernel source files that compile cleanly on host (no inline AArch64 asm) ---
KERNEL_SRC := \
    ../src/drivers/uart/handlers.c \
    ../src/drivers/gic/gic.c \
    ../src/drivers/timer/timer.c \
    ../src/arch/aarch64/kernel/trap/irq.c \
    ../src/arch/aarch64/kernel/trap/panic.c \
    ../src/arch/aarch64/kernel/trap/syscall.c

# --- Host-safe test mocks ---
MOCK_SRC := \
    mocks/mock_gic.c \
    mocks/mock_uart.c

# --- Unit test files (phase numbers match real phase plan) ---
UNIT_SRC := \
    unit/p1-pfa/test_evt_dispatch.c \
    unit/p1-pfa/test_fdt_pfa.c \
    unit/p2-gic-timers/test_gic_timer.c \
    unit/p4-mmu/test_mmu_bitfields.c \
    unit/p5-heap/test_heap.c \
    unit/p6-scheduler/test_scheduler.c \
    unit/p7-syscall/test_syscall.c

# --- Runner + registry ---
RUNNER_SRC := \
    main.c \
    phase_registry.c

# --- Full source list ---
ALL_SRC   := $(RUNNER_SRC) $(MOCK_SRC) $(UNIT_SRC) $(KERNEL_SRC)
ALL_OBJ   := $(patsubst %.c,build/%.o,$(ALL_SRC))

# --- Make targets ---

.PHONY: test test-v clean

test: build/test_runner
	@echo "--------------------------------------------"
	@echo "  Running JOS Unit Tests                    "
	@echo "  (feat/adding-testing-framework branch)     "
	@echo "--------------------------------------------"
	@./build/test_runner

test-v: build/test_runner
	@./build/test_runner --verbose

clean:
	@rm -rf build/

# --- Link the runner ---
build/test_runner: $(ALL_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

# --- Pattern: any .c inside tests/ tree ---
build/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Pattern: any .c in ../src/ tree ---
build/src/%.o: ../src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Terminal output auto-colour when interacting ---
ifeq ($(TERM),)
  GNUMAKEFLAGS += --no-print-directory
endif
