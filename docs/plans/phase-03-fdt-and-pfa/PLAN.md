# Phase 3: Physical Memory (Device Tree & Page Frame Allocator)

## What You're Building

Two things:
1. A **Flattened Device Tree (FDT) parser** to discover how much RAM QEMU gave you.
2. A **Page Frame Allocator (PFA)** to manage that RAM as a pool of 4KB physical pages.

---

## Core Concepts You Must Learn

### 1. The Flattened Device Tree (FDT)

When QEMU boots your kernel, it passes a pointer to the FDT in **register x0**.

#### FDT Structure (in memory)
```
+------------------+
| Header (40 bytes) |
|  magic (0xd00dfeed) |
|  totalsize         |
|  off_dt_struct     |  <- offset to structure block
|  off_dt_strings    |  <- offset to strings block
|  off_mem_rsvmap    |  <- offset to memory reservation block
|  version           |
|  ...               |
+------------------+

+------------------+
| Reserved Memory   |
| (list of regions  |
|  not for general  |
|  use, e.g., DTB   |
|  itself, firmware) |
+------------------+

+------------------+
| Structure Block   |
|  FDT_BEGIN_NODE    |  <- "memory\0"  (a node starts)
|  FDT_PROP (len,nameoff) -> data  <- reg = <base size>
|  FDT_END_NODE      |  <- node ends
|  FDT_END           |  <- end of tree
+------------------+

+------------------+
| Strings Block     |
| "memory\0"        |
| "reg\0"           |
| "compatible\0"    |
| ...               |
+------------------+
```

#### What You Need to Parse
At minimum, find the `/memory` node and extract its `reg` property:
```
/memory {
    device_type = "memory";
    reg = <0x00000000 0x40000000>;   // base=0x0, size=0x40000000 (1GB)
};
```
- The `reg` property is a list of (base, size) pairs. Each is a 64-bit value.
- Endianness: The FDT is **big-endian** in the blob. Your parser must byte-swap on little-endian QEMU.

#### Traversal Pseudocode
```
function parse_fdt(fdt_base):
    header = fdt_base as FDTHeader*
    verify header.magic == 0xd00dfeed (after byte-swap)

    struct_ptr = fdt_base + header.off_dt_struct
    strings_ptr = fdt_base + header.off_dt_strings

    while true:
        token = read_u32(struct_ptr)
        advance struct_ptr by 4

        if token == FDT_BEGIN_NODE:
            name = read_nul_terminated_string(struct_ptr)
            align struct_ptr to 4-byte boundary
            if name == "memory":
                current_node = MEMORY_NODE
        elif token == FDT_PROP:
            len = read_u32(struct_ptr)
            nameoff = read_u32(struct_ptr)
            data_ptr = struct_ptr
            advance struct_ptr by (4 + 4 + len), then align to 4 bytes
            prop_name = strings_ptr + nameoff
            if current_node == MEMORY_NODE and prop_name == "reg":
                base = read_u64(data_ptr)
                size = read_u64(data_ptr + 8)
                memory_base = base; memory_size = size
        elif token == FDT_END_NODE:
            current_node = NONE
        elif token == FDT_END:
            break
```

### 2. Memory Reservations
The FDT header has `off_mem_rsvmap`. Read it!
- Reserved regions include the FDT blob itself, video framebuffers, etc.
- Your PFA must **exclude** these regions from allocation.
- Format: A list of `{ u64 base, u64 size }` ending with `{ 0, 0 }`.

### 3. Page Frame Allocation (PFA)

#### The Goal
Divide available RAM into **4 KiB (4096 byte) chunks** called **page frames**.
Track which frames are free and which are allocated.

#### Simplest Approach: Bitmap Allocator
```
// One bit per page frame.
// 0 = free, 1 = allocated.

// For 1GB RAM: 1,048,576 pages → 131,072 bytes = 128 KiB bitmap.

#define PAGE_SIZE 4096

typedef struct {
    uint64_t base_address;
    uint64_t num_frames;
    uint8_t *bitmap;      // points into kernel static/data area
} pfa_t;

void pfa_init(uint64_t ram_base, uint64_t ram_size, uint64_t kernel_start, uint64_t kernel_end):
    num_frames = ram_size / PAGE_SIZE
    bitmap_size = num_frames / 8

    // Mark ALL frames as allocated initially
    memset(bitmap, 0xFF, bitmap_size)

    // Find first page-aligned address >= kernel_end
    bitmap_base = align_up(kernel_end, PAGE_SIZE)

    // Free the region [bitmap_base + bitmap_size, ram_base + ram_size)
    for each frame in that region:
        pfa_free(frame_address)

// Allocate: scan bitmap for a '0' bit, set to '1', return physical address
uint64_t pfa_alloc():
    for byte in bitmap:
        if byte != 0xFF:
            bit = ctz(~byte)          // count trailing zeros of inverted byte
            set bit in bitmap
            return base_address + ((index * 8 + bit) * PAGE_SIZE)
    return 0   // out of memory

// Free: clear the bit
void pfa_free(uint64_t phys_addr):
    frame_index = (phys_addr - base_address) / PAGE_SIZE
    byte = frame_index / 8
    bit  = frame_index % 8
    clear bit in bitmap[byte]
```

#### Buddy Allocator (More Advanced, Optional)
- The bitmap approach is simple but slow for large allocations.
- For a more advanced project, research the **Buddy System** (used by Linux).
- It maintains power-of-2 free lists (1-page, 2-page, 4-page, etc. blocks).
- Makes it efficient to allocate and free contiguous blocks of pages.

### 4. Linker Script Awareness
- Your kernel occupies RAM from its load address (e.g., `0x40000000`) through its end (`.bss` end).
- The PFA bitmap itself must live inside RAM but outside the kernel image.
- **Important:** The PFA must mark your kernel's own memory as "allocated" so you don't accidentally reuse it.

---

## Step-by-Step Implementation Path

### Step 1: Verify x0 at boot
- In your earliest assembly, move `x0` to a global variable or pass it to C before doing anything else.
- `x0` = pointer to the FDT blob in RAM.
- Print the first 4 bytes. They should be `0xED 0xFE 0x0D 0xD0` (big-endian `0xd00dfeed`).

### Step 2: Write a minimal FDT walker
- Parse the header, validate magic.
- Walk the structure block, tracking only nodes and properties.
- **Don't use recursion.** Use iteration to avoid stack overflow in kernel code.

### Step 3: Extract `/memory` info
- Find the node named `memory`.
- Extract its `reg` property.
- Print: `"RAM: base=0xXXXXXXXX size=0xXXXXXXXX"` via UART.

### Step 4: Parse reserved memory regions
- Read the memory reservation block.
- Mark the FDT blob and any reserved regions as in-use.

### Step 5: Implement the bitmap PFA
- Define a static array for the bitmap (or place it at the end of the kernel image).
- Mark the kernel region + bitmap region + FDT region as allocated.
- Implement `pfa_alloc()` and `pfa_free()`.

### Step 6: Test
```
// Pseudocode test
pfa_init(ram_base, ram_size, kernel_start, kernel_end);

a = pfa_alloc();
b = pfa_alloc();
printf("Allocated pages at 0x%llx and 0x%llx\n", a, b);

pfa_free(a);
c = pfa_alloc();
printf("After free+alloc, page at 0x%llx\n", c);
// Expected: c == a (reused the freed frame)
```

---

## Key Questions to Answer Before Moving On

1. Why is the FDT stored in **big-endian** format even though the CPU is little-endian?
2. What happens if your PFA tries to allocate memory from within its own bitmap?
3. Why is the bitmap approach O(n) for allocation? Is that acceptable for your use case?
4. How do you prevent the PFA from allocating pages that overlap with the UART MMIO region?
5. If QEMU gives you RAM at base=0x40000000 (1GB), but your kernel is loaded there, where should the bitmap live?

---

## Recommended Resources

| Resource | URL | Why Read It |
|----------|-----|-------------|
| **Devicetree Specification** | https://devicetree-specification.readthedocs.io/en/latest/ | Official spec for the FDT blob format, header fields, memory reservation block, and structure block traversal. |
| **Linux Kernel — `libfdt`** | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/scripts/dtc/libfdt | Canonical C library for parsing FDT. Essential for understanding `fdt_next_node()`, `fdt_getprop()`, and address resolution. |
| **Linux Kernel — `Documentation/devicetree/usage-model.rst`** | https://www.kernel.org/doc/Documentation/devicetree/usage-model.rst | Explains how Linux unflattens the device tree and binds drivers. Useful for designing your own FDT walker. |
| **OSDev Wiki — Page Frame Allocation** | https://wiki.osdev.org/Page_Frame_Allocation | Core OSDev article on physical memory management. Covers bitmap allocators, buddy allocators, and integration with bootloader-provided memory maps. |
| **Linux Kernel — `mm/page_alloc.c`** | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/mm/page_alloc.c | Canonical implementation of the buddy page allocator. Reference for zone-aware page frame allocation. |
| **Writing a Simple FDT Parser in C** | Search: "bare metal device tree parser tutorial" | Blog posts showing step-by-step FDT walking. Good for sanity-checking your understanding. |
| **QEMU `virt` Machine Memory Map** | Search: "QEMU virt machine memory layout dtb" | Knows your exact device and RAM layout on QEMU. |
