TARGET    = aarch64-freestanding-none
CC        = zig cc -target $(TARGET)
OBJCOPY   = zig objcopy

INCLUDE_DIRS = -I./src -I./src/arch/aarch64/include -I./src/drivers

CFLAGS = $(INCLUDE_DIRS) -Wall -ffreestanding -mgeneral-regs-only -fno-sanitize=undefined
ASMOPS    = $(CFLAGS)

# Flags for linking ONLY (linker error fixed)
LDFLAGS   = -nostdlib -nostartfiles -T src/linker.ld -Wl,--build-id=none

BUILD_DIR = build
SRC_DIR   = src

C_FILES   = $(shell find $(SRC_DIR) -name '*.c')
ASM_FILES = $(shell find $(SRC_DIR) -name '*.S')
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
DEP_FILES = $(OBJ_FILES:%.o=%.d)

.PHONY: all run clean img

all: run

# Boot in QEMU using the ELF file
run: $(BUILD_DIR)/jos_kernel8.elf
	@echo "Booting in QEMU..."
	qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel $(BUILD_DIR)/jos_kernel8.elf

img: jos.img

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR) jos.img

# Compile C files
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "CC $<"
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Compile ASM files
$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	@echo "AS $<"
	$(CC) $(ASMOPS) -MMD -c $< -o $@

# Link using LDFLAGS
$(BUILD_DIR)/jos_kernel8.elf: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	@mkdir -p $(@D)
	@echo "Linking $@"
	$(CC) $(LDFLAGS) $(OBJ_FILES) -o $@

# Extract binary
jos.img: $(BUILD_DIR)/jos_kernel8.elf
	@echo "Extracting binary $@"
	$(OBJCOPY) -O binary $< $@

-include $(DEP_FILES)
