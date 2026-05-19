#!/bin/bash
set -e

echo "Assembling..."
nasm -f elf32 src/boot.s -o boot.o

echo "Compiling and Linking..."
zig cc -target x86-freestanding \
  -nostdlib \
  -static \
  -fno-sanitize=undefined \
  -fno-pie \
  -mno-red-zone \
  -Wl,--build-id=none \
  -Wl,-z,max-page-size=4096 \
  -T src/linker.ld \
  boot.o src/drivers/vga.c src/kernel.c -o jos.bin

echo "Booting in Terminal..."

qemu-system-i386 -display curses -kernel jos.bin

rm boot.o
