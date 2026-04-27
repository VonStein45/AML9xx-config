#!/bin/sh

# AML-S9XX Configuration Tool Wrapper
# This script bypasses 'noexec' mount restrictions on SD cards by 
# copying the binary to /tmp before execution.
#
# Note: This script usually needs to be run from terminal/konsole prompt
#       (in order to avoid the execution blocking and read the file as text)
#
# .../armbi_boot$ sh ./Launch-AML9xx-config-linux.sh
# or
# .../armbi_boot$ bash ./Launch-AML9xx-config-linux.sh
#

# Identify system architecture
ARCH=$(uname -m)
TOOL_SRC="./AML9xx-config-linux-x64"

if [ "$ARCH" = "aarch64" ]; then
    TOOL_SRC="./AML9xx-config-arm64"
fi

# Define temporary execution path
TMP_TOOL="/tmp/aml-config-tool"

# 1. Check if the binary exists on the SD card
if [ ! -f "$TOOL_SRC" ]; then
    echo "Error: Binary $TOOL_SRC not found in current directory."
    exit 1
fi

# 2. Copy binary to /tmp (where execution is allowed)
cp "$TOOL_SRC" "$TMP_TOOL"
chmod +x "$TMP_TOOL"

# 3. Execution logic: Try normal run, fallback to sudo if needed
echo "Starting configuration tool..."

# Try to run without sudo first
"$TMP_TOOL"
EXIT_CODE=$?

# If exit code 1 (often permission error) or file access fails, retry with sudo
if [ $EXIT_CODE -ne 0 ]; then
    echo ""
    echo "Access denied or operation failed. Retrying with sudo..."
    sudo "$TMP_TOOL"
fi

# 4. Cleanup
rm "$TMP_TOOL"
