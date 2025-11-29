#!/bin/bash

# XFB macOS Build and Package Script
# This script rebuilds XFB for macOS and creates a DMG package

set -e  # Exit on any error

echo "🚀 Starting XFB macOS build process..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="XFB"
VERSION="2.0.0"
BUILD_DIR="build"
APP_NAME="XFB.app"
DMG_NAME="XFB-${VERSION}-macOS"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    print_error "This script is designed for macOS only!"
    exit 1
fi

# Check for required tools
print_status "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake is required but not installed. Please install CMake."
    exit 1
fi

if ! command -v make &> /dev/null; then
    print_error "Make is required but not installed. Please install Xcode Command Line Tools."
    exit 1
fi

if ! command -v hdiutil &> /dev/null; then
    print_error "hdiutil is required but not found. This should be available on macOS."
    exit 1
fi

print_success "All required tools found!"

# Clean previous build if requested
if [[ "$1" == "--clean" ]]; then
    print_status "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
    rm -f "${DMG_NAME}.dmg"
    print_success "Clean completed!"
fi

# Create build directory
print_status "Setting up build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring project with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -DCPACK_GENERATOR=DragNDrop

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed!"

# Build the project
print_status "Building XFB..."
make -j$(sysctl -n hw.ncpu)

if [ $? -ne 0 ]; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully!"

# Check if the app bundle was created
if [ ! -d "bin/${APP_NAME}" ]; then
    print_error "App bundle not found at bin/${APP_NAME}"
    exit 1
fi

print_success "App bundle created at bin/${APP_NAME}"

# Copy the app bundle to the root directory
print_status "Copying app bundle to project root..."
cd ..
rm -rf "$APP_NAME"
cp -R "$BUILD_DIR/bin/$APP_NAME" .

# Verify the app bundle structure
print_status "Verifying app bundle structure..."
if [ ! -f "${APP_NAME}/Contents/MacOS/XFB" ]; then
    print_error "XFB executable not found in app bundle!"
    exit 1
fi

if [ ! -f "${APP_NAME}/Contents/Info.plist" ]; then
    print_error "Info.plist not found in app bundle!"
    exit 1
fi

print_success "App bundle structure verified!"

# Make the executable... executable
chmod +x "${APP_NAME}/Contents/MacOS/XFB"

# Deploy Qt libraries
print_status "Deploying Qt libraries with macdeployqt..."
if command -v macdeployqt &> /dev/null; then
    macdeployqt "$APP_NAME" -verbose=2
    if [ $? -eq 0 ]; then
        print_success "Qt libraries deployed successfully!"
    else
        print_warning "macdeployqt completed with warnings, but continuing..."
    fi
else
    print_warning "macdeployqt not found. The app may require Qt to be installed on target systems."
fi

# Create DMG
print_status "Creating DMG package..."

# Create a temporary directory for DMG contents
DMG_TEMP_DIR="dmg_temp"
rm -rf "$DMG_TEMP_DIR"
mkdir "$DMG_TEMP_DIR"

# Copy app to temp directory
cp -R "$APP_NAME" "$DMG_TEMP_DIR/"

# Create Applications symlink
ln -s /Applications "$DMG_TEMP_DIR/Applications"

# Create the DMG
print_status "Building DMG file..."
hdiutil create -volname "$PROJECT_NAME $VERSION" \
    -srcfolder "$DMG_TEMP_DIR" \
    -ov -format UDZO \
    "${DMG_NAME}.dmg"

if [ $? -ne 0 ]; then
    print_error "DMG creation failed!"
    exit 1
fi

# Clean up temp directory
rm -rf "$DMG_TEMP_DIR"

print_success "DMG created: ${DMG_NAME}.dmg"

# Get file sizes
APP_SIZE=$(du -sh "$APP_NAME" | cut -f1)
DMG_SIZE=$(du -sh "${DMG_NAME}.dmg" | cut -f1)

# Final summary
echo ""
echo "🎉 Build completed successfully!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 App Bundle: $APP_NAME ($APP_SIZE)"
echo "💿 DMG Package: ${DMG_NAME}.dmg ($DMG_SIZE)"
echo "🏗️  Architecture: $(uname -m)"
echo "🎯 Target: macOS 10.15+"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "✅ Ready for distribution!"
echo "   • Test the app: open $APP_NAME"
echo "   • Install from DMG: open ${DMG_NAME}.dmg"
echo ""