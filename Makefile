TARGET  = aarch64-freestanding-none
CC      = zig cc -target $(TARGET)
# -MD and -MP auto-generate header dependency files during compilation
CFLAGS  = -ffreestanding -fno-builtin -nostdlib -O2 -Isrc -mgeneral-regs-only -MD -MP
LDFLAGS = -nostdlib -Wl,-T,src/linker.ld -Wl,--build-id=none -Wl,-z,max-page-size=4096

C_SOURCES = $(shell find src -type f -name "*.c")
S_SOURCES = $(shell find src -type f -name "*.S")

OBJ  = $(C_SOURCES:.c=.o) $(S_SOURCES:.S=.o)
DEPS = $(OBJ:.o=.d)

all: run

# Linker script added as a direct dependency
jos.elf: $(OBJ) src/linker.ld
	@echo "Linking jos.elf..."
	@$(CC) $(LDFLAGS) $(OBJ) -o $@

# C-compile
%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assembly-compile (CFLAGS included to support C macros in assembly)
%.o: %.S
	@echo "AS $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# QEMU start
run: jos.elf
	@echo "Booting in QEMU..."
	@qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel jos.elf

# Rm trash
clean:
	@echo "Cleaning up..."
	@rm -f $(OBJ) $(DEPS) jos.elf

# Include the auto-generated dependency files
-include $(DEPS)

.PHONY: all run clean
