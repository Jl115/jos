# Phase 7: User Space & System Calls (EL0)

## What You're Building

The final barrier: dropping from the all-powerful **EL1 (Kernel Mode)** down to **EL0 (User Mode)**, and implementing a mechanism for user programs to **safely request kernel services** via System Calls (`SVC`).

---

## Core Concepts You Must Learn

### 1. Privilege Levels (AArch64)

| Level | Name               | Access                                                               |
| ----- | ------------------ | -------------------------------------------------------------------- |
| EL3   | Secure Monitor     | Can do everything. TrustZone firmware lives here.                    |
| EL2   | Hypervisor         | Virtualization. Not used in a simple OS.                             |
| EL1   | Kernel / OS        | Full access to MMU, timers, interrupts, physical memory.             |
| EL0   | User / Application | Cannot touch system registers or device MMIO. Must ask EL1 for help. |

#### What EL0 CANNOT do

- Read/write system registers (`MSR`, `MRS` instructions trap).
- Access physical addresses directly (MMU is required; user space only sees VAs).
- Change interrupt masks (`DAIF` bits).
- Access the GIC or UART directly.

#### What EL0 CAN do

- Execute A64 instructions.
- Access its own virtual memory.
- Issue `SVC #imm` to trap to EL1 (a **system call**).

### 2. Dropping to EL0

Your kernel starts in EL1. To create a user process, you must set up an EL0 execution context and then use `eret` to "return" to EL0.

#### Setup before the drop

```
// In the kernel, when creating a user process:
// 1. Allocate a user page table
// 2. Map user code + stack + heap into the user page table
//    - Mark user pages as EL0-accessible (AP[2:1] = 0b01 in page table)
// 3. Set up the user stack
// 4. Configure SPSR_EL1 to describe the target state

// SPSR_EL1 bits:
// [3:0]  M[3:0] = 0b0000  → EL0t (EL0 using SP_EL0)
// [9]    D = 0               Debug mask (clear)
// [8]    A = 0               SError abort mask (clear)
// [7]    I = 0               IRQ mask (clear — allow interrupts in user mode)
// [6]    F = 0               FIQ mask (clear)
// [4]    M[4] = 0            Not using a reserved value
// [0]    NZCV etc. from processor flags

// So: SPSR_EL1 = 0x00000000 means "drop to EL0t, all interrupts unmasked"

// Set ELR_EL1 to the user entry point
msr ELR_EL1, user_entry_point

// Set SP_EL0 to the user stack top
msr SP_EL0, user_stack_top

// Set SPSR_EL1
mov x0, #0x00000000    // EL0t, all unmasked
msr SPSR_EL1, x0

// Set TTBR0_EL1 to the user's page table
msr TTBR0_EL1, user_page_table

// TLB flush (critical!)
dsb ish
tlbi vmalle1is          // invalidate all TLB entries
dsb ish
isb

// ERET!
eret                     // CPU switches to EL0, loads PC from ELR_EL1
```

### 3. System Calls (SVC)

When an EL0 process needs the kernel, it executes:

```
SVC #0      // or any immediate value; the kernel decodes the "syscall number"
```

#### What the hardware does on SVC

1. `SPSR_EL1 = PSTATE` (save current EL0 state)
2. `ELR_EL1 = PC` (save the address of the instruction AFTER `SVC`)
3. PSTATE.M = EL1h (switch to EL1)
4. `SP = SP_EL1` (switch to kernel stack)
5. `PC = VBAR_EL1 + Synchronous_Vector_offset` (jump to the SVC handler)

#### In your SVC handler (Assembly stub)

```
svc_handler:
    // We are at EL1 now.
    // 1. Save user x0-x30 to the kernel stack (or PCB)
    // 2. Extract the syscall number
    //    In AArch64, the syscall number is passed in x8
    //    Arguments: x0-x5
    //    Return value: x0

    // 3. Call the C syscall dispatcher
    //    dispatcher(x8, x0, x1, x2, x3, x4, x5)

    // 4. Restore x0-x30
    // 5. eret (returns to the instruction after SVC in EL0)
```

#### In C: The Syscall Dispatcher

```
#define SYS_PRINT    0
#define SYS_EXIT     1
#define SYS_ALLOC    2

typedef uint64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

syscall_fn_t syscall_table[] = {
    [SYS_PRINT] = sys_print,
    [SYS_EXIT]  = sys_exit,
    [SYS_ALLOC] = sys_alloc,
    ...
};

uint64_t handle_syscall(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5):
    if nr >= NUM_SYSCALLS:
        return -EINVAL
    return syscall_table[nr](a1, a2, a3, a4, a5)
```

#### Syscall arguments (AArch64 ABI convention)

| Register  | Purpose in user→kernel syscall           |
| --------- | ---------------------------------------- |
| **x8**    | Syscall number                           |
| **x0-x5** | Arguments 0–5                            |
| **x0**    | Return value (set by kernel before ERET) |

### 4. System Call: `sys_print`

A user program wants to print `"Hello, World!"` to the UART.

#### User side (EL0 assembly)

```
    adr x0, msg          // pointer to string
    mov x1, #13          // length
    mov x8, #SYS_PRINT   // syscall number
    svc #0               // trap to EL1
    // execution resumes here when the kernel returns
```

#### Kernel side (EL1)

```
uint64_t sys_print(uint64_t user_ptr, uint64_t len, ...):
    // SAFETY CHECK: is user_ptr a valid user-space virtual address?
    // Do NOT blindly dereference user_ptr — it could point to kernel space!
    if !is_user_address(user_ptr):
        return -EFAULT

    // Walk the user's page table to find the physical address
    phys = virt_to_phys_in_process(user_ptr, current->_page_table)
    if phys == 0:
        return -EFAULT

    // Copy len bytes from user-space to a temporary kernel buffer
    copy_from_user(kernel_buf, user_ptr, len)

    // Now safely call uart_print(kernel_buf, len)
    uart_print(kernel_buf, len)
    return 0
```

### 5. Memory Isolation: Kernel vs User Pages

In the page tables:

- **Kernel pages** (e.g., kernel code, UART MMIO): `AP[2:1] = 0b00` (EL1 only, no user access).
- **User pages** (e.g., user code, stack, heap): `AP[2:1] = 0b01` (User R/W, EL1 also R/W).
- **User execute/no-execute:** Set `UXN = 1` for data pages, `UXN = 0` for code pages.

**Critical:** If you accidentally mark a UART page as user-accessible, an EL0 program can directly write to hardware.

### 6. Returning from a Syscall

After the kernel syscall handler finishes, it must:

1. Set `x0` to the return value.
2. Restore all user registers from the saved frame.
3. Execute `eret`.

The `eret` reloads `ELR_EL1` (which is the address of the instruction AFTER `SVC`) and `SPSR_EL1` (which restores the user's PSTATE, including interrupt masks and NZCV flags).

---

## Step-by-Step Implementation Path

### Step 1: Build a user page table

- Map user code and user stack into a separate translation table.
- Ensure kernel pages are marked as EL1-only (`AP = 0b00`).
- Ensure user pages are marked as EL0-accessible (`AP = 0b01`).

### Step 2: Write the drop-to-EL0 sequence

- In `create_user_process()`, set up `SPSR_EL1`, `ELR_EL1`, `SP_EL0`, and `TTBR0_EL1`.
- Use `eret` to "return" to EL0.

### Step 3: Handle an unprivileged exception

- First test: try `svc #0` from EL0 after you drop.
- Your SVT handler at EL1 should catch it.
- Verify: `ESR_EL1.EC == 0b010101` (SVC from AArch64).

### Step 4: Implement the syscall dispatcher

- Build the `syscall_table[]` array.
- Implement `sys_print()` as a safe wrapper around UART.
- Implement `sys_exit()` to mark the process as ZOMBIE.

### Step 5: Build a minimal user library

- Write a small `libc` with `print()`, `exit()`, etc.
- These functions are just thin wrappers that load x8 and execute `svc`.

---

## Key Questions to Answer Before Moving On

1. Why is `SP_EL0` different from `SP_EL1`? What happens if you set `SP_EL0` incorrectly before dropping to EL0?
2. If a user process tries to read from a kernel-only page, what exception does it cause? Which EC value in `ESR_EL1`?
3. What is the **Spectre/Meltdown** risk of blindly copying user data into a kernel buffer? (Hint: speculative execution + side channels.)
4. Why must you invalidate the TLB (`tlbi vmalle1is`) after switching `TTBR0_EL1`?
5. How does the kernel know which process's page table to use during `virt_to_phys_in_process`?

---

## Recommended Resources

| Resource                                                                       | URL                                                                       | Why Read It                                                                                                                                                                                                                              |
| ------------------------------------------------------------------------------ | ------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **ARM Software Interfaces — Procedure Call Standard for AArch64 (AAPCS64)**    | https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst      | Defines the ARM64 calling convention: register roles (`x0–x7` for args/results, `x19–x29` callee-saved), stack alignment, and how data is passed across the EL0↔EL1 boundary.                                                            |
| **ARM Developer — Exception Model / SVC & Exception Handling**                 | https://developer.arm.com/documentation/den0024 (AArch64 Exception Model) | Step-by-step coverage of privilege levels (EL0–EL3), the `SVC #imm` instruction, synchronous exception routing, the role of `VBAR_EL1`, and how the processor state is automatically saved to `SPSR_EL1` / `ELR_EL1` on exception entry. |
| **Raspberry Pi OS Tutorial (Sergey Matyukevich) — Lesson 04+**                 | https://github.com/s-matyukevich/raspberry-pi-os                          | Bare-metal tutorial explicitly implementing the transition from EL1 to EL0, setting up a separate user stack, issuing SVC calls, and writing a minimal syscall handler. One of the best publicly available step-by-step implementations. |
| **Linux Kernel — `arch/arm64/kernel/syscall.c` & `arch/arm64/kernel/entry.S`** | https://github.com/torvalds/linux/tree/master/arch/arm64/kernel           | See how Linux sets up the syscall entry path, handles register clobbering, and dispatches to C from assembly.                                                                                                                            |
| **LWN.net — "SYSCALL ABI and calling conventions" articles**                   | https://lwn.net/search/?q=syscall+ABI                                     | Deep dives into kernel/user boundary mechanics and ABI stability.                                                                                                                                                                        |
| **xv6 RISC-V**                                                                 | Not ARM, but a clear teaching OS                                          | Search: "xv6 book riscv"                                                                                                                                                                                                                 | While not ARM64, xv6 is the canonical teaching OS with excellent explanations of system calls, trap frames, and user/kernel isolation. Swap RISC-V instructions for ARM64 equivalents in your head. |
| **ARM Developer — Learn the Architecture: Exception Model & Security**         | https://developer.arm.com/documentation/102412/latest/                    | High-level ARM tutorial with diagrams explaining privilege transitions, secure vs non-secure, and system call flow.                                                                                                                      |
| **OSDev Wiki — System Calls (general)**                                        | https://wiki.osdev.org/System_Calls                                       | Platform-agnostic explanation of how system calls work: the user/kernel boundary, syscall tables, and argument passing patterns.                                                                                                         |
| **"Spectre and Meltdown" primers**                                             | Search: "Spectre Meltdown ARM64 explanation"                              | Important security context for why you must be careful about reading user-provided pointers in the kernel.                                                                                                                               |
