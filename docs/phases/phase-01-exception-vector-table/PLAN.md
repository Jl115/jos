# Phase 1: Survival (Exception Vector Table)

## What You're Building

The AArch64 Exception Vector Table (EVT). Right now, if your CPU hits a bad memory access or divide-by-zero, it silently freezes. You need to catch these events, understand what happened, and report it via UART.

---

## Core Concepts You Must Learn

### 1. ARM64 Exception Levels (EL)

- **EL3**: The most privileged (Secure Monitor / TrustZone firmware). Usually entered from cold boot.
- **EL2**: Hypervisor (used by virtualization, e.g., KVM). In QEMU `virt`, you may start here.
- **EL1**: Operating System kernel. **This is where your OS lives.**
- **EL0**: User space / applications.
- Learn: How does the CPU know which EL it is currently in? (`CurrentEL` register.)

### 2. The Exception Vector Table Layout

- In AArch64, the **Vector Base Address Register** (`VBAR_EL1`) holds the address of your EVT.
- The table is **not** a jump table of function pointers. It's a block of **assembly code**, 32 instructions (128 bytes) per entry.
- There are **16 entries**, organized by:
  - Exception source: Same EL or Lower EL
  - Exception type: Synchronous, IRQ, FIQ, or SError
- Layout (each group = 4 entries x 128 bytes):
  - Group 0: Current EL with SP_EL0 (SP0)
  - Group 1: Current EL with SP_EL1 (SPx)
  - Group 2: Lower EL using AArch64
  - Group 3: Lower EL using AArch32
- **You will primarily use Groups 1 and 2.**

### 3. Exception Types

| Type            | Cause                                                   | What to learn                                                                        |
| --------------- | ------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| **Synchronous** | Instruction fault, data abort, software breakpoint, SVC | Most common. The exception is directly caused by the instruction that just executed. |
| **IRQ**         | Peripheral interrupt (timer, UART, etc.)                | External, asynchronous.                                                              |

|
| **FIQ** | Fast interrupt (higher priority) | Similar to IRQ but handled with lower latency.
|
| **SError** | System Error (e.g., async parity/ECC fault, external abort) | Rare but fatal. |

### 4. Registers Saved by Hardware on Exception Entry

When an exception occurs, the CPU **automatically** does this for you:

```
SPSR_ELx = PSTATE          // save processor state (flags, bitness, etc.)
ELR_ELx  = PC             // save return address (where to resume)
ESR_ELx  = syndrome info   // WHY this happened (exception class, ISS)
FAR_ELx  = faulting addr   // (only for some sync exceptions) the address that caused it
SP moves to SP_ELx        // stack pointer for the target EL
```

- Learn: What is in `ESR_EL1`? Decode the **Exception Class (EC)** field.
- Learn: What is in `FAR_EL1`? When is it valid vs. unknown/invalid?

### 5. Assembly Code for the Table

Each entry in the EVT must:

1. Save scratch registers (the CPU does NOT save x0–x18 for you).
2. Call a C handler.
3. Restore scratch registers.
4. Execute `eret` to return.

**Pseudocode for one vector entry:**

```
vector_entry_sync_current_el_spx:
    // 1. Save x0-x18 to the stack (or a per-CPU exception frame)
    SUB SP, SP, #(18 * 8)
    STP x0,  x1,  [SP, #0]
    STP x2,  x3,  [SP, #16]
    ... (save all x0-x17)

    // 2. Pass a pointer to the saved frame to C
    MOV x0, SP
    BL  handle_synchronous_exception

    // 3. Restore registers
    LDP x0,  x1,  [SP, #0]
    ... (restore all)
    ADD SP, SP, #(18 * 8)

    // 4. Return from exception
    ERET
```

### 6. C Handler Design

- The C handler receives a pointer to the saved register frame.
- It reads `ESR_EL1` to determine the **Exception Class** (EC).
- It reads `ELR_EL1` to know which instruction caused it.
- For data aborts, it reads `FAR_EL1`.
- It prints diagnostics using `uart_printf`.
- **For unhandled exceptions:** Print a register dump and halt the CPU.

---

## Step-by-Step Implementation Path

### Step 1: Understand where QEMU drops you

- When QEMU boots with `-machine virt -cpu cortex-a72`, which EL do you start in?
- Research: Does QEMU `virt` start at EL3, EL2, or EL1? (Hint: usually EL2 or EL1 depending on `-machine` options.)
- **If EL2:** You may need to drop to EL1 first (covered in Step 7 concepts, Phase 7).
- Verify your current EL by reading the `CurrentEL` system register.

### Step 2: Allocate the EVT in memory

- Reserve exactly 2 KiB (2048 bytes) of aligned memory.
- `VBAR_EL1` requires **2 KiB alignment**.
- Set `VBAR_EL1` to the address of your table.
- **Pseudocode:**
  ```
  .align 11              // 2 KiB alignment (2^11)
  vector_table:
      // 16 entries x 128 bytes each
      // Fill unused entries with a catch-all that dumps and halts
  ```

### Step 3: Write the individual vector handlers

- You need at minimum:
  - **Synchronous exception** from current EL (SPx): for instruction/data aborts, divide-by-zero, etc.
  - **IRQ** from lower EL: for when you have timers running (Phase 2).
- For unused entries: branch to a generic "unhandled exception" dumper.

### Step 4: Write the C diagnostic handler

- Implement a function that decodes `ESR_EL1.EC` into a human-readable string.
- Common EC values to handle: `0x24` (Data Abort from lower EL), `0x25` (Data Abort from current EL), `0x26` (SP Alignment Fault), `0x32` (BRK instruction), etc.
- Print: EC name, `ELR_EL1` (faulting instruction address), `FAR_EL1` (if valid), and register dump.

### Step 5: Force an exception and verify

- After setting up the EVT, deliberately cause a data abort:
  ```
  MOV x0, #0xDEADBEEF_BAD_ADDR     // some address in unmapped space
  LDR x1, [x0]                     // this should trigger a sync abort
  ```
- You should see your handler catch it and print via UART before halting.

---

## Key Questions to Answer Before Moving On

1. What is the difference between a Synchronous Abort and an SError?
2. Why must each vector entry be exactly 128 bytes? What happens if your handler is longer?
3. If the CPU automatically saves the return address in `ELR_EL1`, why must you still save x0-x18 manually?
4. What does the `DAIF` field in `SPSR_EL1` control, and why is it important?
5. What happens if an exception occurs while you're already handling an exception? (Hint: think about stack overflow and nested exceptions.)

---

## Recommended Resources

| Resource                                                                                         | URL                                                                                               | Why Read It                                                                                             |
| ------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| **OSDev Wiki — ARM64 Exception Handling** (if it exists, also see ARM64 Bare Bones)              | https://wiki.osdev.org/ARM_Architecture                                                           | Community-curated bare-metal guidance. Start here for the big picture.                                  |
| **ARM Architecture Reference Manual for ARMv8-A (DDI 0487)** — Chapter D1 (AArch64 System Level) | https://developer.arm.com/documentation/ddi0487/latest/                                           | The **only** authoritative source for how exceptions work, what each register does, and exact encoding. |
| **ARM Cortex-A Series Programmer’s Guide (DEN0024)** — Exception Model chapter                   | https://developer.arm.com/documentation/den0024/latest/                                           | Easy-to-read explanation of exception levels, types, and vector table layout.                           |
| **Raspberry Pi OS Tutorial (Sergey Matyukevich) — Lesson 01-02**                                 | https://github.com/s-matyukevich/raspberry-pi-os                                                  | Step-by-step bare-metal boot on real hardware. Shows actual assembly for the EVT.                       |
| **Linux Kernel — `arch/arm64/kernel/entry.S`**                                                   | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/arch/arm64/kernel/entry.S | Production-grade exception entry/exit. Don't copy-paste, but see _what_ Linux saves and restores.       |
| **Linux Kernel — `arch/arm64/kernel/traps.c`**                                                   | https://git.kernel.org/.../arch/arm64/kernel/traps.c                                              | See how Linux decodes `ESR_EL1` and `FAR_EL1` in C. Great reference for your diagnostic functions.      |
| **AArch64 ESR_EL1 Exception Class Reference**                                                    | Search: "AArch64 ESR_EL1 exception class table"                                                   | You need a quick reference for EC values when decoding.                                                 |
| **ARM Developer — Learn the Architecture: Exception Model**                                      | https://developer.arm.com/documentation/102412/latest/                                            | High-level ARM tutorial with diagrams and examples.                                                     |
