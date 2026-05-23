# Phase 4: Virtual Memory (MMU & Paging)

## What You're Building

The **Memory Management Unit (MMU)** configuration for AArch64. You will construct **Translation Tables** (page tables) that map virtual addresses to physical addresses. This is the hardest phase. A single mistake in the MMU registers or table entries will cause an immediate crash.

---

## Core Concepts You Must Learn

### 1. Why Virtual Memory?

| Problem | How Virtual Memory Fixes It |
|---------|-----------------------------|
| Processes use same physical addresses and crash into each other | Each process gets its own virtual address space |
| Fragmented physical memory | The OS can scatter a process's pages physically while appearing contiguous virtually |
| A process can write anywhere in RAM | The MMU enforces permissions (read/write/execute/user/kernel) on every page |
| No way to isolate user-space from kernel | Kernel pages can be marked as kernel-only |

### 2. AArch64 Address Translation Overview

- **Virtual Address (VA)** → **Intermediate Physical Address (IPA)** → **Physical Address (PA)**.
- At EL1, Stage 1 translates VA → IPA. (On a simple OS without a hypervisor, IPA == PA.)
- Translation tables are **hierarchical**. For 4 KiB pages with 48-bit VAs, there are **4 levels** (Level 0 → 1 → 2 → 3).

#### Translation Table Levels (48-bit, 4 KiB granule)
```
VA[47:39]  →  Translation Table Level 0 (L0)  → points to L1 table address
VA[38:30]  →  Translation Table Level 1 (L1)  → points to L2 table address
VA[29:21]  →  Translation Table Level 2 (L2)  → points to L3 table address
VA[20:12]  →  Translation Table Level 3 (L3)  → page descriptor (final 4 KiB page)
VA[11:0]   →  Offset within the 4 KiB page
```

Each table is **4096 bytes** with **512 entries** (each entry = 8 bytes).

#### Descriptor Types (at each level)

| Bits | Meaning |
|------|---------|
| `[1:0]` | `00` = Invalid, `01` = Block (huge page), `11` = Table (pointer to next level) |
| At L0-L2 | Can be Block or Table. If Block, maps a **huge region**. |
| At L3     | Must be a **Page** descriptor (final 4 KiB mapping). |

#### Page Descriptor Layout (L3 entry, 8 bytes)
```
[47:12]  Output address bits [47:12] of the physical page
[11:10]  Reserved (must be 0)
[9]      AF (Access Flag)
[8:7]    AP[2:1] (Access Permissions: R/W, User/Privileged)
[6]      Reserved
[5]      SH (Shareability)
[4:2]    AF (Access Flag — set to 1 or the CPU will fault on first access)
[1:0]    11 = valid page descriptor
[63:59]  RES0
[58:55]  Contiguous hint (optional)
[54]     UXN/XN (Execute-never)
[10]     APTable (subsequent levels)
[...]    See ARM manual for full bitfield
```
- **AF = Access Flag:** Set to 1. If 0, the CPU raises a permission fault on first access.
- **AP bits:** Control user/kernel and read/write access.
- **XN bit:** Mark pages as non-executable (essential for security).

### 3. Memory Attributes (MAIR, TCR)

#### MAIR_EL1 (Memory Attribute Indirection Register)
- This register holds **8 attribute encodings** (indices 0–7).
- Each index defines a memory type: Normal (cached), Device-nGnRnE, etc.
- Example:
  ```
  // Attribute 0 = Normal Inner/Outer Write-Back Cacheable
  MAIR_ATTR0 = 0xFF   // Inner: Write-Back, Outer: Write-Back

  // Attribute 1 = Device nGnRnE (strict ordering, no caching)
  MAIR_ATTR1 = 0x00

  // MAIR_EL1 = (ATTR1 << 8) | ATTR0 = 0x00000000000000FF
  ```
- The **AttrIndx** field (bits [4:2]) in each page table entry selects which MAIR attribute applies.

#### TCR_EL1 (Translation Control Register)
- Configures the address translation regime.
- Key fields:
  - `T0SZ` (bits [5:0]): Size offset for TTBR0_EL1 region (e.g., `0x10` = 48-bit VA).
  - `IRGN0`/`ORGN0` (bits [9:8], [11:10]): Inner/Outer cacheability for table walks.
  - `SH0` (bits [13:12]): Shareability for table walks.
  - `TG0` (bit [14]): Granule size (0 = 4 KiB).
  - `IPS` (bits [34:32]): Intermediate Physical Space size.
  - `AS` (bit [36]): ASID size.
  - `TBI0` (bit [37]): Top Byte Ignore (for pointer tagging, can ignore for now).
  - `EPD1` (bit [23]): Disable TTBR1_EL1 translations (kernel space upper half).

- **Configuration for a simple identity-mapped kernel:**
  ```
  // T0SZ = 16  →  64 - 16 = 48-bit VA space
  // TG0 = 0   →  4 KiB granule
  // EPD1 = 1  →  Disable upper half (TTBR1 unsupported for now)
  ```

### 4. Identity Mapping vs Kernel Mapping

| Approach | Description |
|----------|-------------|
| **Identity Mapping** | Virtual Address == Physical Address for everything. Simple. No need to rewrite pointers. But also no protection between kernel and user. |
| **Higher Half Mapping** | Kernel lives at a high virtual address (e.g., `0xFFFF000000000000`), while user space is in the lower half. This is what Linux does. |

For Phase 4, **start with identity mapping** to avoid relocation issues. You can later switch to higher-half mapping.

### 5. Turn On The MMU (The Scary Part)

Once you set `SCTLR_EL1.M = 1`, the MMU is live. **Every memory access is now translated.**

#### Pseudocode for MMU enable sequence
```
// 1. Allocate page tables in identity-mapped memory
//    (before the MMU is on, memory accesses use physical addresses)
//    Make sure the page tables themselves are in RAM you control.

// 2. Set up L0, L1, L2, L3 tables
//    For a simple identity map, map RAM as Normal Memory and UART as Device Memory.
//    You can use huge blocks (1 GiB or 2 MiB) for big contiguous regions to save space.

// 3. Configure MAIR_EL1
mov x0, #0x00FF   // Attr0=Normal WB, Attr1=Device nGnRnE
msr MAIR_EL1, x0

// 4. Configure TCR_EL1
tcr_value = (16 << 0)         // T0SZ = 16
          | (0 << 14)         // TG0 = 4 KiB
          | (1 << 23)         // EPD1 = 1 (disable TTBR1)
          | (0b11 << 8)       // IRGN0 = Inner WB cacheable
          | (0b11 << 10)      // ORGN0 = Outer WB cacheable
          | (0b11 << 12)      // SH0 = Inner Shareable
msr TCR_EL1, x0

// 5. Write TTBR0_EL1 with the address of your L0 table
msr TTBR0_EL1, x_table_addr

// 6. Ensure all table writes are visible (DSB + ISB)
dsb ish
isb

// 7. Enable the MMU: SCTLR_EL1 = SCTLR_EL1 | 1
mrs x0, SCTLR_EL1
orr x0, x0, #1
msr SCTLR_EL1, x0
isb

// 8. If you identity-mapped cleanly, execution continues normally
// 9. Congratulations, you're now in virtual memory!
```

#### After the MMU is on
- Your stack pointer `SP` still holds a physical address. Is it identity-mapped? It better be, or your next push/pop will fault.
- If you mapped code at VA=0x40000000 and PA=0x40000000, great — identity map works.
- If you mapped code at VA=0xFFFF000000000000 but loaded at PA=0x40000000, then `pc` would need to be updated. That's "higher half mapping" and is more advanced.

---

## Step-by-Step Implementation Path

### Step 1: Design your page table layout
- Decide: Will you use identity mapping or higher-half?
- For identity mapping: Allocate physical RAM for L0-L3 tables before the MMU turns on.
- Use a linker script or hard-code a region past your kernel image.

### Step 2: Implement table manipulation helpers
```
// Pseudocode
void set_l0_entry(uint64_t *l0_table, uint64_t va, uint64_t l1_phys_addr):
    idx = (va >> 39) & 0x1FF
    l0_table[idx] = l1_phys_addr | 0b11   // valid + table descriptor

void set_l1_block_entry(uint64_t *l1_table, uint64_t va, uint64_t pa, uint64_t attr):
    // Maps a 1 GiB block (if granule is 4 KiB)
    idx = (va >> 30) & 0x1FF
    l1_table[idx] = pa | attr | 0b01      // valid + block descriptor

void set_l3_page_entry(uint64_t *l3_table, uint64_t va, uint64_t pa, uint64_t attr):
    // Maps a 4 KiB page
    idx = (va >> 12) & 0x1FF
    l3_table[idx] = pa | attr | 0b11      // valid + page descriptor
```

### Step 3: Build the initial identity map
- Map RAM region (`0x40000000` — `0x80000000`) as Normal Memory.
- Map UART (`0x09000000`) as Device Memory (nGnRnE).
- Map your kernel code as Read-Execute (no write).
- Map your kernel data/bss/stack as Read-Write (no execute).

### Step 4: Configure MAIR, TCR, TTBR
- Fill `MAIR_EL1` with Normal and Device attributes.
- Fill `TCR_EL1` with correct T0SZ, granule, and cacheability settings.
- Write `TTBR0_EL1` with your L0 table's physical address.

### Step 5: Enable the MMU
- Do the `SCTLR_EL1` enable dance.
- Add an `isb` after enabling.
- Immediately after: print a character via UART. If it works, the MMU survived.

### Step 6: Implement page mapping functions
```
// Map a single 4 KiB page
void map_page(uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap a page
void unmap_page(uint64_t virt);

// Get physical address for a virtual address (useful for debugging)
uint64_t virt_to_phys(uint64_t virt);
```

---

## Key Questions to Answer Before Moving On

1. What happens if you enable the MMU but forget to identity-map the code currently executing?
2. Why is the **Access Flag (AF)** important? What happens if AF=0?
3. What is the difference between **Device Memory** and **Normal Memory**? Why can't you just mark everything as Normal?
4. If you want to map a 1 GiB region, could you use a **Block** descriptor at L1 instead of 262,144 L3 Page descriptors? What are the tradeoffs?
5. What does `ISB` (Instruction Synchronization Barrier) do, and why must you issue it after enabling the MMU?

---

## Recommended Resources

| Resource | URL | Why Read It |
|----------|-----|-------------|
| **ARM Architecture Reference Manual (DDI 0487)** — Memory Management / Translation chapters | https://developer.arm.com/documentation/ddi0487/latest/ | **The** definitive reference for TCR, MAIR, translation table descriptor formats, and MMU behavior. |
| **ARM Learn the Architecture — Memory Management** | https://developer.arm.com/documentation/102412/latest/ | Accessible, high-level coverage of AArch64 address translation, table walks, cacheability/shareability, and memory attribute indexing. |
| **OSDev Wiki — ARM64 Page Table** | https://wiki.osdev.org/ARM64-PageTable | OSdev-focused practical guide to setting up ARM64 initial page tables, mapping regions, and enabling the MMU from bare-metal. |
| **Linux Kernel Docs — ARM64 Memory Layout** | https://docs.kernel.org/arch/arm64/memory.html | Describes the kernel/user virtual address split, translation table levels as used by Linux, and how memory types are mapped. |
| **Raspberry Pi OS Tutorial — Lesson 06 (MMU)** | https://github.com/s-matyukevich/raspberry-pi-os | Hands-on implementation of ARM64 MMU setup on real hardware. Shows actual descriptors and barrier usage. |
| **Linux Kernel — `arch/arm64/mm/proc.S`** | https://git.kernel.org/.../arch/arm64/mm/proc.S | See how Linux writes MAIR_EL1, TCR_EL1, and TTBR0_EL1 before enabling the MMU in early boot. |
| **Linux Kernel — `arch/arm64/mm/mmu.c`** | https://git.kernel.org/.../arch/arm64/mm/mmu.c | Kernel page table construction. Very dense but shows all descriptor attributes. |
| **MMU Cacheability: ARM White Paper** | Search: "ARM memory attributes cacheability shareability" | Understand how Inner/Outer cacheability and shareability interact with multi-core systems. |
