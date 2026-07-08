#!/bin/bash

# XFB Build Verification Script
# This script verifies the built app and DMG

set -e

echo "🔍 Verifying XFB macOS build..."

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

print_check() {
    echo -e "${BLUE}[CHECK]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Check if app bundle exists
print_check "Checking XFB.app bundle..."
if [ -d "XFB.app" ]; then
    print_success "XFB.app bundle found"
else
    print_error "XFB.app bundle not found"
    exit 1
fi

# Check executable
print_check "Checking executable..."
if [ -f "XFB.app/Contents/MacOS/XFB" ] && [ -x "XFB.app/Contents/MacOS/XFB" ]; then
    print_success "XFB executable found and is executable"
else
    print_error "XFB executable not found or not executable"
    exit 1
fi

# Check Info.plist
print_check "Checking Info.plist..."
if [ -f "XFB.app/Contents/Info.plist" ]; then
    print_success "Info.plist found"
else
    print_error "Info.plist not found"
    exit 1
fi

# Check icon
print_check "Checking app icon..."
if [ -f "XFB.app/Contents/Resources/XFB.icns" ]; then
    print_success "App icon found"
else
    print_error "App icon not found"
    exit 1
fi

# Check Qt frameworks
print_check "Checking Qt frameworks..."
if [ -d "XFB.app/Contents/Frameworks" ] && [ "$(ls -1 XFB.app/Contents/Frameworks/Qt*.framework 2>/dev/null | wc -l)" -gt 0 ]; then
    framework_count=$(ls -1 XFB.app/Contents/Frameworks/Qt*.framework 2>/dev/null | wc -l)
    print_success "Found $framework_count Qt frameworks"
else
    print_error "Qt frameworks not found"
    exit 1
fi

# Check plugins
print_check "Checking Qt plugins..."
if [ -d "XFB.app/Contents/PlugIns" ] && [ "$(find XFB.app/Contents/PlugIns -name '*.dylib' 2>/dev/null | wc -l)" -gt 0 ]; then
    plugin_count=$(find XFB.app/Contents/PlugIns -name '*.dylib' 2>/dev/null | wc -l)
    print_success "Found $plugin_count Qt plugins"
else
    print_error "Qt plugins not found"
    exit 1
fi

# Check DMG
print_check "Checking DMG file..."
if [ -f "XFB-3.14159-macOS.dmg" ]; then
    dmg_size=$(du -h "XFB-3.14159-macOS.dmg" | cut -f1)
    print_success "DMG file found (${dmg_size})"
else
    print_error "DMG file not found"
    exit 1
fi

# Get file sizes
app_size=$(du -sh XFB.app | cut -f1)
dmg_size=$(du -sh XFB-3.14159-macOS.dmg | cut -f1)

echo ""
echo "📊 Build Summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 App Bundle: XFB.app ($app_size)"
echo "💿 DMG Package: XFB-3.14159-macOS.dmg ($dmg_size)"
echo "🏗️  Architecture: $(file XFB.app/Contents/MacOS/XFB | grep -o 'arm64\|x86_64')"
echo "🎯 Target: macOS 10.15+"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "✅ All checks passed! Build is ready for distribution."
echo ""
echo "📋 Next steps:"
echo "   • Test the app: open XFB.app"
echo "   • Install from DMG: open XFB-3.14159-macOS.dmg"
echo "   • Distribute the DMG file to users"
echo ""