#!/bin/bash
# Script for building a shadowOS userspace, uses: https://github.com/shadowOS-dev/bootstrap
set -e

srcdir="$(dirname "$0")"
test -z "$srcdir" && srcdir=.
cd "$srcdir"

ROOT=$(pwd)/..
BOOTSTRAP_DIR="$ROOT/bootstrap"
STRAP="${STRAP:-xbstrap}"

REQUIRED_FILES=(
    "bin/*"
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

# Actually bootstrap shadowOS, build all packages
pushd "$BOOTSTRAP_DIR"
$STRAP init .
$STRAP install --all

mkdir -p "$ROOT/distro-files"

if [ -d "system-root/" ]; then
    echo "Copying selected files..."
    for pattern in "${REQUIRED_FILES[@]}"; do
        matches=($(find "system-root/" -path "system-root/$pattern"))
        
        if [ ${#matches[@]} -eq 0 ]; then
            echo "Warning: No files matched '$pattern'"
        fi

        for file in "${matches[@]}"; do
            dest="$ROOT/distro-files/${file#system-root/}"
            mkdir -p "$(dirname "$dest")"
            cp "$file" "$dest"
        done
    done

    for pattern in "${REQUIRED_FILES[@]}"; do
        if ! find "$ROOT/distro-files" -path "$ROOT/distro-files/$pattern" | grep -q .; then
            echo "Error: Missing required file(s) matching: $pattern"
            exit 1
        fi
    done
else
    echo "Warning: system-root/ not found, nothing copied."
    exit 1
fi
popd
