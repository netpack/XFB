#!/bin/bash

# XFB Development Launch Script
# This script sets up the proper Qt environment for development

# Set Qt plugin path
export QT_PLUGIN_PATH="/opt/homebrew/share/qt/plugins"

# Launch XFB
if [ -f "XFB.app/Contents/MacOS/XFB" ]; then
    echo "Launching XFB app bundle..."
    
    # Fix permissions and re-sign if needed
    if [ ! -w "XFB.app/Contents/Frameworks/libbrotlicommon.1.dylib" ]; then
        echo "Fixing permissions and re-signing app bundle..."
        chmod -R u+w XFB.app/Contents/Frameworks/ 2>/dev/null || true
        codesign --force --deep --sign - XFB.app 2>/dev/null || true
    fi
    
    open XFB.app
else
    echo "App bundle not found. Please run build-macos.sh first."
    exit 1
fi