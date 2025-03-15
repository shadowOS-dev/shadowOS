#!/bin/bash
cd "$(dirname "$0")" || exit 1

compile_and_link() {
    local source_file="$1"
    local output_file="$2"
    
    gcc -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -nostdlib -c "$source_file" -o "$output_file.o"
    ld "$output_file.o" -o "$output_file" -T linker.ld -nostdlib --no-dynamic-linker --strip-all
}

compile_and_link "init.c" "init"