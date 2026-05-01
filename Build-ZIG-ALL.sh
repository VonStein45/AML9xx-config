#!/bin/bash

# Change directory to script location
cd "$(dirname "$0")"

# Define source and build options
SRC="AML9xx-config.c"
OPTS="-O ReleaseSmall -fstrip -fsingle-threaded -lc --color off"

echo "Building Windows x86... (32-bit x86)"
zig build-exe $SRC -target x86-windows-gnu $OPTS --name AML9xx-config-win-x86

echo "Building Linux x64 (Static musl)..."
zig build-exe $SRC -target x86_64-linux-musl $OPTS --name AML9xx-config-linux-x64

echo "Building ARM64 (Static musl)..."
zig build-exe $SRC -target aarch64-linux-musl $OPTS --name AML9xx-config-arm64

echo
echo "NB: Absolute alpha-test builds of the two flavours of MAC OS binaries.."
echo "    These are not even tested to run, and not LIPO packed and/or signed. I.e. they are ONLY for test purposes."
echo "Additional note: -fstrip might cause MAC to fail if binary is not signed."
echo

echo "Building ARM64 Apple Silicon (M1-M4)..."
zig build-exe $SRC -target aarch64-macos $OPTS --name AML9xx-config-macos-arm64

echo "Building Intel Mac..."
zig build-exe $SRC -target x86_64-macos $OPTS --name AML9xx-config-macos-x86_64

echo "Done!"

# Pause equivalent: Wait for a single key press
read -n1 -r -p "Press any key to continue..."

