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
VERSION="3.14159"
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
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -DBUILD_TESTING=OFF \
    -DCPACK_GENERATOR=DragNDrop

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed!"

# Build the project
print_status "Building XFB..."
make XFB -j$(sysctl -n hw.ncpu)

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
    # Run macdeployqt without letting "set -e" abort the whole script if it
    # returns non-zero. macdeployqt exits with an error when it can't resolve an
    # rpath for a plugin we don't even ship (e.g. the SVG icon engine, which
    # references QtSvg.framework). By that point the main frameworks are already
    # deployed, and we remove the SVG plugins further below, so this is benign —
    # but under "set -e" the bare error code would otherwise kill the script
    # before re-signing and DMG creation ever run.
    macdeployqt_rc=0
    macdeployqt "$APP_NAME" -verbose=2 || macdeployqt_rc=$?
    if [ "$macdeployqt_rc" -ne 0 ]; then
        print_warning "macdeployqt returned ${macdeployqt_rc} (typically an unresolved rpath for a plugin we don't ship, e.g. QtSvg). Continuing with bundle fix-ups."
    fi
    # Proceed with the fix-ups as long as macdeployqt got far enough to create
    # the Frameworks directory (the SVG-plugin error happens at the very end,
    # after all real frameworks are already copied).
    if [ -d "${APP_NAME}/Contents/Frameworks" ]; then
        print_success "Qt libraries deployed!"
        
        # Fix missing dependencies that macdeployqt might miss
        print_status "Fixing missing dependencies..."
        
        # Detect Homebrew prefix (works on both Intel and Apple Silicon)
        BREW_PREFIX="$(brew --prefix 2>/dev/null || echo "/opt/homebrew")"
        
        # Copy missing brotli library
        if [ -f "${BREW_PREFIX}/opt/brotli/lib/libbrotlicommon.1.dylib" ]; then
            cp "${BREW_PREFIX}/opt/brotli/lib/libbrotlicommon.1.dylib" "${APP_NAME}/Contents/Frameworks/"
            # Fix the library path
            install_name_tool -change "@rpath/libbrotlicommon.1.dylib" "@executable_path/../Frameworks/libbrotlicommon.1.dylib" "${APP_NAME}/Contents/Frameworks/libbrotlidec.1.dylib" 2>/dev/null || true
        fi
        
        # Copy QtDBus framework if missing
        if [ -d "${BREW_PREFIX}/opt/qtbase/lib/QtDBus.framework" ] && [ ! -d "${APP_NAME}/Contents/Frameworks/QtDBus.framework" ]; then
            cp -R "${BREW_PREFIX}/opt/qtbase/lib/QtDBus.framework" "${APP_NAME}/Contents/Frameworks/"
        fi
        
        print_success "Dependencies fixed!"
        
        # Fix Qt framework paths to prevent conflicts with system Qt
        print_status "Fixing Qt framework paths to use bundled frameworks..."
        EXECUTABLE="${APP_NAME}/Contents/MacOS/XFB"
        
        if [ -f "$EXECUTABLE" ]; then
            # First, add the rpath to the bundled frameworks
            print_status "Adding rpath to bundled frameworks..."
            install_name_tool -add_rpath "@executable_path/../Frameworks" "$EXECUTABLE" 2>/dev/null || true
            # Get current library paths and fix them
            otool -L "$EXECUTABLE" | grep -E "(homebrew|opt)" | while read -r line; do
                # Extract the path (first field)
                old_path=$(echo "$line" | awk '{print $1}')
                
                # Extract framework name from the path and create new path
                if [[ "$old_path" =~ QtMultimedia\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtMultimedia.framework/Versions/A/QtMultimedia"
                elif [[ "$old_path" =~ QtQuickWidgets\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQuickWidgets.framework/Versions/A/QtQuickWidgets"
                elif [[ "$old_path" =~ QtWidgets\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets"
                elif [[ "$old_path" =~ QtWebEngineQuick\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtWebEngineQuick.framework/Versions/A/QtWebEngineQuick"
                elif [[ "$old_path" =~ QtWebEngineCore\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtWebEngineCore.framework/Versions/A/QtWebEngineCore"
                elif [[ "$old_path" =~ QtQuick\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQuick.framework/Versions/A/QtQuick"
                elif [[ "$old_path" =~ QtOpenGL\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtOpenGL.framework/Versions/A/QtOpenGL"
                elif [[ "$old_path" =~ QtGui\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui"
                elif [[ "$old_path" =~ QtConcurrent\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtConcurrent.framework/Versions/A/QtConcurrent"
                elif [[ "$old_path" =~ QtQmlMeta\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQmlMeta.framework/Versions/A/QtQmlMeta"
                elif [[ "$old_path" =~ QtQmlModels\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQmlModels.framework/Versions/A/QtQmlModels"
                elif [[ "$old_path" =~ QtQmlWorkerScript\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQmlWorkerScript.framework/Versions/A/QtQmlWorkerScript"
                elif [[ "$old_path" =~ QtWebChannel\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtWebChannel.framework/Versions/A/QtWebChannel"
                elif [[ "$old_path" =~ QtQml\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtQml.framework/Versions/A/QtQml"
                elif [[ "$old_path" =~ QtNetwork\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtNetwork.framework/Versions/A/QtNetwork"
                elif [[ "$old_path" =~ QtSql\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtSql.framework/Versions/A/QtSql"
                elif [[ "$old_path" =~ QtPositioning\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtPositioning.framework/Versions/A/QtPositioning"
                elif [[ "$old_path" =~ QtCore\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore"
                elif [[ "$old_path" =~ QtWebChannelQuick\.framework ]]; then
                    new_path="@executable_path/../Frameworks/QtWebChannelQuick.framework/Versions/A/QtWebChannelQuick"
                else
                    continue
                fi
                
                install_name_tool -change "$old_path" "$new_path" "$EXECUTABLE" 2>/dev/null || true
            done
            
            # Verify the fix
            remaining=$(otool -L "$EXECUTABLE" | grep -E "(homebrew|opt)" | wc -l)
            if [ "$remaining" -eq 0 ]; then
                print_success "Qt framework paths fixed successfully!"
            else
                print_warning "Some Homebrew paths may still remain, but continuing..."
            fi
        else
            print_warning "Executable not found for Qt path fixing"
        fi
    else
        print_error "macdeployqt did not produce a Frameworks directory; the bundle is incomplete."
        exit 1
    fi
else
    print_warning "macdeployqt not found. The app may require Qt to be installed on target systems."
fi

# Fix the QtWebEngineProcess helper. macdeployqt frequently leaves this helper
# binary pointing at Homebrew Qt frameworks by absolute path; on a Mac without
# Homebrew Qt those paths don't exist and WebEngine (the in-app browser used for
# torrent search) fails to start. Rewrite its framework references to @rpath and
# point its rpath at the bundle's Frameworks directory.
WEBENGINE_HELPER="${APP_NAME}/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess"
if [ -f "$WEBENGINE_HELPER" ]; then
    print_status "Fixing QtWebEngineProcess framework paths..."
    # From the helper's MacOS dir up to XFB.app/Contents/Frameworks is 7 levels.
    install_name_tool -add_rpath "@executable_path/../../../../../../../Frameworks" "$WEBENGINE_HELPER" 2>/dev/null || true
    otool -L "$WEBENGINE_HELPER" | grep -E "/opt/(homebrew|local)" | awk '{print $1}' | while read -r old_path; do
        case "$old_path" in
            *.framework/*)
                fw=$(echo "$old_path" | sed -E 's#.*/([A-Za-z0-9_]+\.framework/.*)#\1#')
                install_name_tool -change "$old_path" "@rpath/${fw}" "$WEBENGINE_HELPER" 2>/dev/null || true
                ;;
        esac
    done
    remaining_helper=$(otool -L "$WEBENGINE_HELPER" | grep -cE "/opt/(homebrew|local)" || true)
    if [ "$remaining_helper" -eq 0 ]; then
        print_success "QtWebEngineProcess paths fixed."
    else
        print_warning "QtWebEngineProcess still has $remaining_helper external path(s)."
    fi
fi

# Fix the brotli library's own install-name so it isn't an absolute Homebrew path.
BROTLI_COMMON="${APP_NAME}/Contents/Frameworks/libbrotlicommon.1.dylib"
if [ -f "$BROTLI_COMMON" ]; then
    install_name_tool -id "@executable_path/../Frameworks/libbrotlicommon.1.dylib" "$BROTLI_COMMON" 2>/dev/null || true
fi

# Remove unused Qt plugins that reference frameworks we don't bundle. The app
# uses no SVG assets and no on-screen keyboard, so these plugins only produce
# "Cannot resolve rpath" warnings and dead weight. Removing them keeps the
# bundle clean and self-consistent.
print_status "Removing unused plugins (SVG, virtual keyboard)..."
rm -f "${APP_NAME}/Contents/PlugIns/iconengines/libqsvgicon.dylib"
rm -f "${APP_NAME}/Contents/PlugIns/imageformats/libqsvg.dylib"
rm -f "${APP_NAME}/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib"

# Re-sign the bundle (ad-hoc). This MUST happen after all install_name_tool
# edits and plugin removals above — those modifications invalidate the
# signatures macdeployqt applied. Nested code (the QtWebEngineProcess helper
# app and the frameworks) must be signed from the inside out; "--deep" alone
# does not reliably re-sign a nested .app, so sign the helper explicitly first.
print_status "Re-signing the app bundle (ad-hoc, inside-out)..."
HELPER_APP="${APP_NAME}/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
# 1. Loose plugins and dylibs.
find "${APP_NAME}/Contents/PlugIns" -name "*.dylib" -exec codesign --force --sign - {} \; 2>/dev/null || true
find "${APP_NAME}/Contents/Frameworks" -maxdepth 1 -name "*.dylib" -exec codesign --force --sign - {} \; 2>/dev/null || true
# 2. The self-contained WebEngine helper app (deep is appropriate here).
[ -d "$HELPER_APP" ] && codesign --force --deep --sign - "$HELPER_APP" 2>/dev/null || true
# 3. Each bundled framework, so its seal includes the (now-signed) helper.
for fw in "${APP_NAME}/Contents/Frameworks/"*.framework; do
    [ -d "$fw" ] && codesign --force --sign - "$fw" 2>/dev/null || true
done
# 4. Finally the main executable and the outer app. No "--deep" here: the
#    contents are already signed, and --deep would redo nested code and can
#    leave the WebEngine helper inconsistent.
codesign --force --sign - "${APP_NAME}/Contents/MacOS/XFB" 2>/dev/null || true
codesign --force --sign - "$APP_NAME" 2>/dev/null || true
if codesign --verify --deep --strict "$APP_NAME" 2>/dev/null; then
    print_success "Code signature is valid."
else
    print_warning "Code signature could not be verified; the app may warn on first launch."
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
echo "🎯 Target: macOS 11.0+"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "✅ Ready for distribution!"
echo "   • Test the app: open $APP_NAME"
echo "   • Install from DMG: open ${DMG_NAME}.dmg"
echo ""