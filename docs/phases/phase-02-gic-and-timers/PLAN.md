# Phase 2: Hardware Awareness (GIC & Timers)

## What You're Building

You're adding a "heartbeat" to your OS. You'll initialize the **Generic Interrupt Controller (GIC)** and the **ARM Generic Timer** so that hardware interrupts fire at a regular interval. Your EVT from Phase 1 will catch these IRQs.

---

## Core Concepts You Must Learn

### 1. The Generic Interrupt Controller (GIC)

The GIC is the bridge between peripheral devices and the CPU.

#### GICv2 vs GICv3
- **GICv2**: Distributor + CPU Interface. The CPU interface is memory-mapped.
- **GICv3**: Adds Redistributors. CPU interface accessed via system registers (`ICC_*_EL1`) instead of MMIO.
- **What QEMU uses?** `virt` machine typically uses **GICv3** (or GICv2 when forced with `-machine virt,gic_version=2`).
- **Pseudocode to detect:**
  ```
  // GICv3 Distributor has a register at offset 0x0000 called GICD_TYPER
  // Read it to find out how many SPIs (Shared Peripheral Interrupts) are supported
  // and whether the GIC supports security extensions.
  ```

#### GIC Components
| Component | Role |
|-----------|------|
| **Distributor** | Routes interrupts to CPUs. Manages priority, grouping (Group 0 vs Group 1). |
| **Redistributor** | (GICv3 only) Per-CPU. Manages PPIs (Private Peripheral Interrupts) and SGIs (Software Generated Interrupts). |
| **CPU Interface** | Per-CPU. Signals interrupts to the core. Handles priority masking and acknowledgement. |

#### Interrupt Types
| Type | Abbreviation | Routed By |
|------|--------------|-----------|
| Software Generated | SGI | Distributor |
| Private Peripheral | PPI | Redistributor (per-CPU) |
| Shared Peripheral | SPI | Distributor |
| Locality-specific Peripheral | LPI | GICv3 ITS (not needed for a simple OS) |

### 2. The ARM Generic Timer

Each ARMv8 core has a built-in timer, independent of the GIC.

- **Counter:** `CNTVCT_EL0` — a free-running 64-bit counter that always increments at a fixed frequency.
- **Frequency:** `CNTFRQ_EL0` — read this to know how many ticks per second.
- **Virtual Timer:** `CNTV_TVAL_EL0` + `CNTV_CTL_EL0` — the one you use in EL1.
- **Physical Timer:** `CNTP_TVAL_EL0` + `CNTP_CTL_EL0` — typically used by EL2 hypervisors.

#### How It Works
1. Read `CNTFRQ_EL0` → e.g., 62,500,000 Hz (62.5 MHz on QEMU `virt`).
2. Decide your interval, e.g., 1ms = 62,500 ticks.
3. Write that value to `CNTV_TVAL_EL0`. This loads a countdown value.
4. Set `CNTV_CTL_EL0.IMASK = 0` (unmask) and `CNTV_CTL_EL0.ENABLE = 1`.
5. When the countdown reaches 0, it triggers a **timer PPI** (usually IRQ 30 on GICv3, IRQ 27 on GICv2).
6. Your GIC routes this to the CPU.
7. Your CPU takes the IRQ exception via your EVT.
8. **In your IRQ handler:**
   - Acknowledge the interrupt in the GIC CPU Interface (`ICC_IAR1_EL1`).
   - Your handler does work (e.g., increment a tick counter).
   - Signal End of Interrupt (`ICC_EOIR1_EL1`).
   - Write a new value to `CNTV_TVAL_EL0` to re-arm the timer.

### 3. The Full Interrupt Lifecycle

```
[Timer expires]
    ↓
[GIC Distributor receives timer PPI]
    ↓
[GIC CPU Interface signals CPU: "IRQ pending!"]
    ↓
[CPU checks PSTATE.I (Interrupt mask bit)]
    ↓
[If unmasked: CPU takes IRQ exception]
    ↓
[Hardware saves context → ELR_EL1, SPSR_EL1]
    ↓
[Jumps to VBAR_EL1 + IRQ_vector_offset]
    ↓
[Your assembly stub saves x0-x18]
    ↓
[Calls C handler: handle_irq()]
    ↓
[handle_irq() reads ICC_IAR1_EL1 → gets interrupt ID]
    ↓
[Does work...]
    ↓
[handle_irq() writes ICC_EOIR1_EL1 → signals done]
    ↓
[Re-arms timer if needed]
    ↓
[Returns from C]
    ↓
[Assembly stub restores x0-x18]
    ↓
[ERET → resumes interrupted code]
```

---

## Step-by-Step Implementation Path

### Step 1: Map the GIC MMIO regions
On QEMU `virt`, the GIC is at known addresses in the Device Tree (Phase 3). For now, hard-code:
- GIC Distributor base: typically `0x08000000` (varies, check QEMU docs or device tree).
- GIC Redistributor base: `0x080A0000` (GICv3).
- **Pseudocode:**
  ```
  #define GICD_BASE  0x08000000
  #define GICR_BASE  0x080A0000

  gicd = (volatile uint32_t *) GICD_BASE;
  gicr = (volatile uint32_t *) GICR_BASE;
  ```

### Step 2: Initialize the GIC Distributor
- Enable the Distributor (`GICD_CTLR`).
- Configure the Group and priority for the timer interrupt.
- Set the interrupt as non-secure Group 1 if running non-secure.
- **Pseudocode:**
  ```
  // GICv3 Distributor init
  gicd[GICD_CTLR / 4] = 0x0;          // disable first
  wait_for_rwp(gicd);                   // wait for write pending
  // Configure timer IRQ as Group 1, non-secure
  // Set priority (lower value = higher priority)
  // Enable the Distributor
  gicd[GICD_CTLR / 4] = GICD_CTLR_EnableGrp1_NS;
  ```

### Step 3: Initialize the GIC Redistributor (GICv3) or CPU Interface (GICv2)
- Wake up the Redistributor.
- Set PPI priorities.
- Enable the CPU interface.
- Unmask IRQs at the CPU interface level.

### Step 4: Initialize the ARM Generic Timer
- Read `CNTFRQ_EL0`.
- Program `CNTV_TVAL_EL0` with your tick interval.
- Enable the virtual timer (`CNTV_CTL_EL0`).

### Step 5: Enable IRQs in the CPU (PSTATE.I)
- Unmask the I bit in `DAIF`.
- In assembly: `msr DAIFClr, #0x2`  (bit 1 = IRQ unmask).

### Step 6: Write the IRQ handler
- In your EVT IRQ entry, call a C function `handle_irq()`.
- `handle_irq()`:
  1. Reads `ICC_IAR1_EL1` → gets the **interrupt ID**.
  2. If ID == 30 (or timer PPI), increments a global `ticks` counter.
  3. Writes `ICC_EOIR1_EL1`.
  4. Re-arms the timer.
- Test: You should see `ticks` incrementing in a loop, visible via UART.

---

## Key Questions to Answer Before Moving On

1. Why are there two timers (Virtual and Physical)? Which one should EL1 use?
2. What is the difference between `ICC_IAR1_EL1` and `ICC_EOIR1_EL1`?
3. What happens if you forget to re-arm the timer after handling the IRQ?
4. Why must you wait for the GIC Distributor's RWP (Register Write Pending) bit before continuing initialization?
5. If you have multiple CPUs (SMP), what changes about GIC initialization?

---

## Recommended Resources

| Resource | URL | Why Read It |
|----------|-----|-------------|
| **OSDev Wiki — ARM Generic Interrupt Controller** | https://wiki.osdev.org/ARM_Generic_Interrupt_Controller | OSDev-focused guide covering register basics, interrupt types, and minimal GICv2 initialization. |
| **ARM GICv3/v4 Architecture Specification (IHI 0069)** | https://developer.arm.com/documentation/ihi0069/latest/ | The official architecture spec. Covers Distributor, Redistributor, CPU interface, ITS, and security-state initialization. |
| **ARM Cortex-A Series Programmer's Guide (DEN0024)** — Interrupts and Timers chapters | https://developer.arm.com/documentation/den0024/latest/ | Practical chapters on interrupts and timers. Best starting point for understanding GICv2 vs GICv3 and how to configure the Generic Timer from EL1/EL2. |
| **ARM Architecture Reference Manual (DDI 0487)** — Generic Timer & GIC sections | https://developer.arm.com/documentation/ddi0487/latest/ | Authoritative reference for all system registers (`ICC_*`, `ICR_*`, `CNT*_*_EL0`) and memory-mapped layouts. |
| **Linux Kernel — `drivers/irqchip/irq-gic-v3.c`** | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/irqchip/irq-gic-v3.c | Production-quality GICv3 driver. Use as reference for exact MMIO sequences, EOImode handling, and affinity routing. |
| **Raspberry Pi OS Tutorial — Lesson 05 (IRQs & Timer)** | https://github.com/s-matyukevich/raspberry-pi-os | Practical bare-metal implementation of the GIC and timer on real ARM hardware. |
| **QEMU `virt` Machine Device Tree Bindings** | Search: "QEMU virt dtb GIC base address" | Helps you find the exact GIC MMIO base addresses for QEMU. |
| **ARM Generic Timer Reference** | Search: "ARM Generic Timer CNTFRQ CNTV_CTL_EL0" | Quick reference for timer register semantics. |
