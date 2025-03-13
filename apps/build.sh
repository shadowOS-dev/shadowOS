#!/bin/bash
cd "$(dirname "$0")" || exit 1

# test.c
gcc -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -nostdlib -c test.c -o test.o
ld test.o -o test -T linker.ld -nostdlib --no-dynamic-linker --strip-all
