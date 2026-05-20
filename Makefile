TARGET  = aarch64-freestanding-none
CC      = zig cc -target $(TARGET)
CFLAGS  = -ffreestanding -fno-builtin -nostdlib -O2 -Isrc
LDFLAGS = -nostdlib -Wl,-T,src/linker.ld -Wl,--build-id=none -Wl,-z,max-page-size=4096

C_SOURCES = $(shell find src -type f -name "*.c")
S_SOURCES = $(shell find src -type f -name "*.S")

OBJ = $(C_SOURCES:.c=.o) $(S_SOURCES:.S=.o)

all: run

# Link
jos.elf: $(OBJ)
	@echo "Linking jos.elf..."
	@$(CC) $(LDFLAGS) $(OBJ) -o $@

# C-compile
%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assembly-compile
%.o: %.S
	@echo "AS $<"
	@$(CC) -c $< -o $@

# QEMU start
run: jos.elf
	@echo "Booting in QEMU..."
	@qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel jos.elf

# Rm trash
clean:
	@echo "Cleaning up..."
	@find src -name "*.o" -type f -delete
	@rm -f jos.elf

.PHONY: all run clean
