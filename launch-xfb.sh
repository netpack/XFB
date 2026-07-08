#!/bin/bash

# XFB macOS Launcher Script
# This script properly sets up the environment for running XFB on macOS

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$SCRIPT_DIR/src/XFB.app/Contents/MacOS/XFB"
PLUGIN_PATH="$SCRIPT_DIR/src/XFB.app/Contents/PlugIns"

echo "XFB macOS Launcher"
echo "=================="

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: XFB executable not found at $EXECUTABLE"
    echo "Please build the application first using build-macos.sh"
    exit 1
fi

# Check if plugins directory exists
if [ ! -d "$PLUGIN_PATH" ]; then
    echo "Error: Qt plugins directory not found at $PLUGIN_PATH"
    exit 1
fi

# Set Qt plugin path
export QT_PLUGIN_PATH="$PLUGIN_PATH"

echo "Starting XFB..."
echo "Executable: $EXECUTABLE"
echo "Plugin Path: $QT_PLUGIN_PATH"
echo ""

# Launch XFB with any provided arguments
"$EXECUTABLE" "$@"