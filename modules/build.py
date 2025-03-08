#!/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

MODULES = ['test']
SRC_DIR = Path('modules')
OBJ_DIR = SRC_DIR / 'obj'
MOD_DIR = SRC_DIR / 'mod'

CC = 'cc'
CFLAGS = ['-std=c99', '-Wall', '-nostdlib', '-ffreestanding', '-O2', '-fPIC']
LDFLAGS = ['-nostdlib', '-ffreestanding']

def ensure_dir(directory):
    directory.mkdir(parents=True, exist_ok=True)

def compile_module(module_name):
    module_path = SRC_DIR / module_name
    src_files = list(module_path.glob("*.c"))
    obj_files = []
    
    module_obj_dir = OBJ_DIR / module_name
    ensure_dir(module_obj_dir)
    ensure_dir(MOD_DIR)

    for src_file in src_files:
        obj_file = module_obj_dir / f"{src_file.stem}.o"
        print(f"Compiling {src_file}...")
        subprocess.run([CC] + CFLAGS + ['-c', str(src_file), '-o', str(obj_file)], check=True)
        obj_files.append(obj_file)

    elf_file = MOD_DIR / f"{module_name}.elf"
    print(f"Linking {obj_files} into {elf_file}...")
    subprocess.run([CC] + LDFLAGS + [str(obj) for obj in obj_files] + ['-o', str(elf_file), '-T', str(module_path / 'linker.ld')], check=True)

def clean_module(module_name):
    module_path = SRC_DIR / module_name
    obj_files = (OBJ_DIR / module_name).glob(f"{module_name}*.o")
    elf_file = MOD_DIR / f"{module_name}.elf"

    for obj_file in obj_files:
        if obj_file.exists():
            obj_file.unlink()
    if elf_file.exists():
        elf_file.unlink()

def clean_all():
    for module_name in MODULES:
        clean_module(module_name)

def build_all():
    for module_name in MODULES:
        compile_module(module_name)

def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <build|clean>")
        sys.exit(1)

    action = sys.argv[1].lower()
    if action == 'build':
        build_all()
    elif action == 'clean':
        clean_all()
    else:
        print("Unknown action:", action)
        sys.exit(1)

if __name__ == '__main__':
    main()
