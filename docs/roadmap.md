### Phase 1: Survival (Exception Vector Table)

Right now, if you divide by zero or access a bad pointer, your CPU throws a "Synchronous Abort" and freezes forever. You are programming completely blind.

- **The Task:** Build the AArch64 Exception Vector Table (EVT) in Assembly.
- **The Goal:** Catch exceptions (Sync, IRQ, FIQ, SError) and route them to a C handler that prints the exact error and the memory address that caused it using your `uart_printf`.

### Phase 2: Hardware Awareness (GIC & Timers)

Your OS is currently deaf. It cannot hear the outside world.

- **The Task:** Initialize the Generic Interrupt Controller (GICv2 or GICv3, depending on QEMU). Then, program the ARM Generic Timer.
- **The Goal:** Your OS receives hardware interrupts. You can make the timer fire every millisecond, triggering an IRQ that your EVT catches and handles. This is the "heartbeat" of the OS.

### Phase 3: Physical Memory (Device Tree & PFA)

You don't actually know how much RAM QEMU gave you.

- **The Task:** Write a parser for the Flattened Device Tree (FDT). QEMU passes the memory address of the FDT in register `x0` when booting. Read it to find the `/memory` node.
- **The Goal:** Build a Page Frame Allocator (PFA). Divide the available RAM into 4KB blocks. Write functions to allocate and free physical pages.

### Phase 4: Virtual Memory (MMU & Paging)

This will be the hardest part of the project. A modern OS does not run on physical addresses.

- **The Task:** Build ARM64 Translation Tables (Level 0 through 3). Configure the `MAIR_EL1`, `TCR_EL1`, and `TTBR0_EL1` registers.
- **The Goal:** Turn on the MMU. Map your RAM as "Normal Memory" and your UART address (`0x09000000`) as strict "Device Memory" (nGnRnE). If you fail here, the OS instantly crashes.

### Phase 5: Dynamic Memory (Kernel Heap)

Now that you have virtual memory, you need standard data structures.

- **The Task:** Write a memory allocator.
- **The Goal:** Implement a working `kmalloc()` and `kfree()` so your kernel can create linked lists, process queues, and dynamic strings.

### Phase 6: Multitasking & Scheduling

An OS must run multiple programs.

- **The Task:** Define a "Process Control Block" (PCB). Write an Assembly routine to perform a Context Switch.
- **The Goal:** Use the timer from Phase 2 to interrupt Process A, save all its registers (`x0-x30`, `sp_el0`, `elr_el1`, `spsr_el1`) to the stack, and load the registers of Process B.

### Phase 7: User Space & System Calls (EL0)

Right now, your code runs in Exception Level 1 (Kernel Mode) with absolute power.

- **The Task:** Drop the privilege level from EL1 down to EL0 (User Mode).
- **The Goal:** A user program tries to print text. Since it doesn't have access to the UART hardware, it triggers a System Call (`SVC` instruction), forcing the kernel to print the text on its behalf safely.
