#!/bin/bash
# Script for building a shadowOS userspace, uses: https://github.com/shadowOS-dev/bootstrap
set -e

srcdir="$(dirname "$0")"
test -z "$srcdir" && srcdir=.
cd "$srcdir"

ROOT=$(pwd)/..
BOOTSTRAP_DIR="$ROOT/bootstrap"
STRAP="${STRAP:-xbstrap}"

if ! command -v "$STRAP" &> /dev/null; then
    echo "Warning: '$STRAP' not found in PATH. Please install it or specify another path via STRAP."
    exit 1
fi

if [ ! -d "$BOOTSTRAP_DIR" ]; then
    echo "Cloning bootstrap repository..."
    git clone --depth=1 https://github.com/shadowOS-dev/bootstrap "$BOOTSTRAP_DIR" || { echo "Failed to clone bootstrap"; exit 1; }
else
    echo "Bootstrap directory already exists. Pulling latest changes..."
    pushd "$BOOTSTRAP_DIR"
    git pull --ff-only || { echo "Failed to pull latest changes"; exit 1; }
    popd
fi

# Actually bootstrap shadowOS, build all packages
pushd "$BOOTSTRAP_DIR"
$STRAP init .
$STRAP install --all # Builds and installs every package

# Populate distro-files with the newly built sys root
mkdir -p "$ROOT/distro-files"
if [ -d "system-root/" ]; then
    cp -r system-root/* "$ROOT/distro-files/"
fi
popd
