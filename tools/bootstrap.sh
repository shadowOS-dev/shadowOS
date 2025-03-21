#!/bin/bash
# Script for building a shadowOS userspace, uses: https://github.com/shadowOS-dev/bootstrap
set -e

ROOT=$(pwd)
srcdir="$(dirname "$0")"
test -z "$srcdir" && srcdir=.
cd "$srcdir"

BOOTSTRAP_DIR="$ROOT/bootstrap"
STRAP="${STRAP:-xbstrap}"

# Define required files as tuples (input, output)
REQUIRED_FILES=(
    "system-root/bin/init:/bin/init"
)

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

mkdir -p $ROOT/distro-files

# Actually bootstrap shadowOS, build all packages
pushd "$BOOTSTRAP_DIR"
$STRAP init .
$STRAP install --all

mkdir -p "$ROOT/distro-files"

echo "Copying selected files..."
for tuple in "${REQUIRED_FILES[@]}"; do
    IFS=":" read -r src dst <<< "$tuple"
    
    if [ ! -f "$src" ]; then
        echo "Warning: Source file '$src' not found"
        continue
    fi
    
    dest="$ROOT/distro-files$dst"
    mkdir -p "$(dirname "$dest")"
    cp "$src" "$dest"
    echo "Copied '$src' to '$dest'"
done

for tuple in "${REQUIRED_FILES[@]}"; do
    IFS=":" read -r src dst <<< "$tuple"

    if [ ! -f "$ROOT/distro-files$dst" ]; then
        echo "Error: Missing required file(s) matching: $dst"
        exit 1
    fi
done
popd
