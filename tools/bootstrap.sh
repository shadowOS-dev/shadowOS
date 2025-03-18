#!/bin/bash
# Script for building a shadowOS userspace, uses: https://github.com/shadowOS-dev/bootstrap
set -e

BOOTSTRAP_DIR="bootstrap"
STRAP="${STRAP:-xbstrap}"
ROOT=$(pwd)

if ! command -v "$STRAP" &> /dev/null; then
    echo "Warning: '$STRAP' not found in PATH. Please install it or specify another path via STRAP."
    exit 1
fi

if [ ! -d "$BOOTSTRAP_DIR" ]; then
    echo "Cloning bootstrap repository..."
    git clone --depth=1 https://github.com/shadowOS-dev/bootstrap "$BOOTSTRAP_DIR" || { echo "Failed to clone bootstrap"; exit 1; }
else
    echo "Bootstrap directory already exists. Skipping clone."
fi

# Actually bootstrap shadowOS, build all packages
pushd "$BOOTSTRAP_DIR"
$STRAP init .
$STRAP install --all # Builds and installs every package

# Populate distro-files with the newly built sys root
cp -r system-root/* $ROOT/distro-files/
popd