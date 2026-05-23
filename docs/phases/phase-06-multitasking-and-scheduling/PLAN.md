# Phase 6: Multitasking & Scheduling

## What You're Building

**Real multitasking.** More than one process running concurrently. You will:

1. Define a **Process Control Block (PCB)** to hold a process's state.
2. Write an **Assembly context switch** routine to save one process's registers and restore another's.
3. Use the **timer from Phase 2** to trigger a scheduler that decides which process runs next.

---

## Core Concepts You Must Learn

### 1. The Process Control Block (PCB)

The PCB is the kernel's record of everything about a process. When a process is not running, ALL of its state lives here.

```
typedef enum { READY, RUNNING, BLOCKED, ZOMBIE } proc_state_t;

typedef struct pcb {
    // Identifier
    uint32_t pid;
    char name[16];
    proc_state_t state;

    // Saved CPU state (what you save/restore during context switch)
    uint64_t x[31];        // x0 - x30 (x30 is LR)
    uint64_t sp;           // stack pointer (SP_EL0 or SP_EL1, depending on design)
    uint64_t pc;           // elr_el1 (next instruction to execute)
    uint64_t pstate;       // spsr_el1 (saved processor state)

    // Memory management
    uint64_t *page_table;  // pointer to process's L0 translation table

    // Scheduling
    uint64_t ticks_used;
    uint64_t priority;
    struct pcb *next;      // linked list for scheduler queues
} pcb_t;
```

- **x0-x30:** General-purpose registers.
- **sp:** The process's stack pointer. At this stage, your kernel is in EL1 so this is SP_EL1.
- **pc:** `ELR_EL1` — the exception link register, which is where the process resumes after `eret`.
- **pstate:** `SPSR_EL1` — saved processor state (flags, interrupt mask, exception level).

### 2. The Context Switch (in Assembly)

The scheduler decides which process runs next. The context switch is the **mechanical act** of swapping register banks.

#### When does a context switch happen?

- **Voluntary:** A process calls `yield()` or blocks on I/O.
- **Involuntary (Preemptive):** The timer fires every N milliseconds, the timer IRQ handler calls the scheduler.

#### What the hardware does on exception entry (IRQ)

When a timer IRQ fires:

```
SPSR_EL1  = PSTATE (current proc's processor state)
ELR_EL1   = PC     (current proc's instruction pointer)
SP        = SP_EL1 (kernel stack)
CPSR: DAIF = masked (interrupts disabled)
PC jumps to VBAR_EL1 + IRQ_offset
```

So the hardware **already saved pc and pstate.** You only need to save x0-x30 and sp.

#### Assembly context switch pseudocode

```
save_current_context:
    // Assume x0 points to the CURRENT process's PCB
    // We are in the IRQ handler's assembly stub.
    // The stub already saved x0-x18 on the stack.
    // Now we save them into the PCB AND save x19-x30 as well.

    // Get current PCB pointer (assumes some kernel global or passed in x0)
    ldr x0, =current_pcb
    ldr x0, [x0]

    // Save x0-x30. Note: x0-x18 are ON THE STACK.
    // We need to pull them off the stack and write them into the PCB.
    add x1, x0, PCB_OFFSET_X0
    ldp x2, x3, [sp, #16*0]    // load x0, x1 from stack
    stp x2, x3, [x1, #16*0]    // store to pcb->x[0], x[1]
    ... (repeat for x2-x18)

    // x19-x30 are still in registers (they were not touched by the stub)
    // Save them directly from registers into the PCB
    stp x19, x20, [x1, #19*8]
    ...
    stp x29, x30, [x1, #29*8]

    // Save SP (which is SP_EL1, the kernel stack pointer for this process)
    mov x2, sp
    str x2, [x0, PCB_OFFSET_SP]

    // The hardware already saved PC into ELR_EL1 and PSTATE into SPSR_EL1
    mrs x2, ELR_EL1
    str x2, [x0, PCB_OFFSET_PC]
    mrs x2, SPSR_EL1
    str x2, [x0, PCB_OFFSET_PSTATE]

    // Switch translation tables if using per-process virtual memory
    // (Phase 6 may not need this initially — use one kernel page table)

    return

load_next_context:
    // x0 points to the NEXT process's PCB
    ldr x1, [x0, PCB_OFFSET_SP]
    mov sp, x1                // restore stack pointer

    // Restore x19-x30 directly from PCB into registers
    add x1, x0, PCB_OFFSET_X0
    ldp x19, x20, [x1, #19*8]
    ...
    ldp x29, x30, [x1, #29*8]

    // Restore ELR_EL1 and SPSR_EL1 (hardware will use these on ERET)
    ldr x2, [x0, PCB_OFFSET_PC]
    msr ELR_EL1, x2
    ldr x2, [x0, PCB_OFFSET_PSTATE]
    msr SPSR_EL1, x2

    // x0-x18 will be restored from the stack by the assembly stub just before ERET
    // The stub does: ldp x0, x1, [sp, #0] etc.

    // Done. When ERET runs, the CPU loads PC from ELR_EL1 and PSTATE from SPSR_EL1.
    // Execution resumes in the process.
```

**Alternative strategy:** Instead of saving x0-x18 from the stack into the PCB, you can have the assembly stub NOT save x0-x18 to the stack at all. Instead, save them directly into the PCB. This is cleaner but requires a custom assembly stub per entry path.

### 3. The Scheduler

Simplest scheduler: **Round-Robin.**

```
// A circular linked list of READY processes
current = head_pcb;

void schedule(void):
    // Called from the timer IRQ handler, or from a yield syscall
    // Mark current as READY (if it was RUNNING)
    if current != NULL and current->state == RUNNING:
        current->state = READY

    // Pick next process
    next = current->next
    if next == NULL:
        next = head_pcb   // wrap around

    // Simple: just rotate
    next->state = RUNNING
    current = next

    // Return to the assembly stub, which calls save/load
```

**More advanced schedulers:** Priority queues, multi-level feedback queues, CFS (Completely Fair Scheduler — what Linux uses).

### 4. The Process Stack

Each process needs its own stack. At boot:

```
pcb_t *create_process(void *entry_point):
    p = kmalloc(sizeof(pcb_t))
    p->.pid = next_pid++
    p->state = READY

    // Allocate a kernel stack
    stack_page = pfa_alloc()     // 4 KiB stack
    p->sp = stack_page + PAGE_SIZE - 8   // start at top of page

    p->pc = entry_point
    p->pstate = 0b0101   // EL1h mode, IRQ unmasked (DAIF = 0000)

    // Link into ready list
    append_to_ready_list(p)
    return p
```

- When the process first runs, the scheduler sets `ELR_EL1 = entry_point`.
- The `eret` will "return" to the entry point.
- The process runs with its own SP.

---

## Step-by-Step Implementation Path

### Step 1: Define the PCB struct

- Decide exactly what goes in the PCB.
- Allocate a fixed-size array of PCBs (e.g., 64 processes max).

### Step 2: Implement process creation

- `create_process(entry_point)` allocates a PCB and a stack page.
- Sets initial register values to zero, `pc = entry_point`, `sp = stack_top`.

### Step 3: Implement a simple scheduler

- Maintain a linked list of READY processes.
- `schedule()` simply rotates to the next one.

### Step 4: Write the context switch in assembly

- In `entry.S`, extend your IRQ stub:
  1. Save x0-x18 (current registers) to a temporary area or directly into the PCB.
  2. Call `schedule()` in C.
  3. `schedule()` sets a global `current_pcb` to the next process.
  4. Back in assembly: load all registers from the new PCB.
  5. `eret`.

### Step 5: Test with two processes

- Create two idle-ish processes that just increment a counter and print their PID.
- You should see both processes taking turns, triggered by the timer every 1ms.

---

## Key Questions to Answer Before Moving On

1. If the hardware already saves `ELR_EL1` and `SPSR_EL1` on IRQ entry, why must your context switch save them AGAIN into the PCB?
2. What happens if a process's stack overflows and overwrites its own PCB?
3. Why are x19-x30 called "callee-saved" registers? Does that matter for context switching?
4. If your scheduler picks the same process to run again, should you skip the full save/restore to save time?
5. What happens to your `current_pcb` global during nested interrupts? Could scheduling inside an IRQ cause deadlock?

---

## Recommended Resources

| Resource                                                                              | URL                                                                       | Why Read It                                                                                                                                                                          |
| ------------------------------------------------------------------------------------- | ------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **ARM Architecture Reference Manual (DDI 0487)** — Processor State, PSTATE, SPSR, ELR | https://developer.arm.com/documentation/ddi0487/latest/                   | Canonical reference for ARM64 processor state, general-purpose registers, system registers (PSTATE, SPSR, ELR), and the exact mechanics of preserving/restoring execution context.   |
| **OSDev Wiki — ARM64 Bare Bones & Scheduling**                                        | https://wiki.osdev.org/ARM_Architecture https://wiki.osdev.org/Scheduling | Community-curated bare-metal OS development guides. Covers basic register conventions, interrupt handling, and scheduler concepts on ARM64.                                          |
| **Linux Kernel — `arch/arm64/kernel/process.c` & `entry.S`**                          | https://github.com/torvalds/linux/tree/master/arch/arm64/kernel           | Production-grade implementation of ARM64 `cpu_switch_mm()` and `kernel_thread()` / `ret_to_user`. Shows how the kernel saves/restores GPRs, LR, SP, and the TPIDR on context switch. |
| **ARM Learn the Architecture — Register Banking and Context Switching**               | https://developer.arm.com/documentation/102412/0100                       | Official ARM educational content explaining register banking, stack selection per exception level, and the role of system registers during mode transitions.                         |
| **LWN.net — "The Linux Scheduler" & context-switch internals**                        | https://lwn.net/Kernel/Index/#Scheduling                                  | High-quality articles explaining how the generic scheduling algorithm connects to the architecture-specific assembly switch code.                                                    |
| **Raspberry Pi OS Tutorial — Lesson 07 (Processes)**                                  | https://github.com/s-matyukevich/raspberry-pi-os                          | Practical bare-metal implementation of process creation and context switching on real ARM hardware.                                                                                  |
| **MIT 6.828 (xv6) — Context Switch Lecture**                                          | Search: "xv6 context switch lecture notes"                                | Not ARM64, but the concepts of PCB, scheduler, and context switch are universal. Very clear lecture notes.                                                                           |
| **ARM Software Interfaces — Procedure Call Standard for AArch64 (AAPCS64)**           | https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst      | Defines which registers are caller-saved vs callee-saved. Essential for deciding what your context switch must preserve.                                                             |
