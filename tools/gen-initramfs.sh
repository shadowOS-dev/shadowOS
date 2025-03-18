#!/bin/bash
INITRAMFS=$1
OUT=ramfs.img

set -e

if [[ -d "$INITRAMFS" ]]; then
    rm -rf $OUT
    tar -cvf "$OUT" -C "$INITRAMFS" .
else
    exit 1
fi