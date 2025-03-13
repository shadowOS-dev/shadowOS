#!/bin/bash
cd "$(dirname "$0")" || exit 1

# test.c
gcc -ffreestanding -nostdlib -c test.c -o test.o
ld test.o -o test -T linker.ld -nostdlib
