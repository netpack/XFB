#!/bin/bash
# Test script for XFB installation on Arch-based systems
# This script helps verify the package works correctly on Arch Linux

set -e

echo "=========================================="
echo "XFB Arch Linux Installation Test Script"
echo "=========================================="
echo ""

# Check if running on Arch-based system
if ! command -v pacman &> /dev/null; then
    echo "❌ Error: This script is for Arch-based systems only"
    exit 1
fi

echo "✓ Detected Arch-based system"
echo ""

# Function to check if running in a VM or container
check_environment() {
    echo "Checking environment..."
    if systemd-detect-virt &> /dev/null; then
        VIRT=$(systemd-detect-virt)
        echo "✓ Running in: $VIRT"
    else
        echo "✓ Running on bare metal"
    fi
    echo ""
}

# Update system
update_system() {
    echo "Step 1: Updating system..."
    sudo pacman -Syu --noconfirm
    echo "✓ System updated"
    echo ""
}

# Install base dependencies
install_dependencies() {
    echo "Step 2: Installing build dependencies..."
    sudo pacman -S --needed --noconfirm \
        base-devel \
        git \
        cmake \
        pkg-config \
        qt6-base \
        qt6-multimedia \
        qt6-webengine \
        qt6-quickwidgets \
        at-spi2-core \
        speech-dispatcher \
        alsa-lib \
        libpulse \
        sqlite \
        curl \
        gstreamer \
        gst-plugins-base \
        gst-plugins-good
    echo "✓ Build dependencies installed"
    echo ""
}

# Install optional dependencies
install_optional_deps() {
    echo "Step 3: Installing optional dependencies..."
    sudo pacman -S --needed --noconfirm \
        orca \
        brltty \
        espeak-ng \
        ffmpeg \
        lame \
        sox \
        flac \
        vorbis-tools \
        wavpack \
        opus-tools \
        mediainfo \
        pulseaudio-alsa
    echo "✓ Optional dependencies installed"
    echo ""
}

# Build and install from PKGBUILD
build_and_install() {
    echo "Step 4: Building XFB from PKGBUILD..."
    
    # Create temporary build directory
    BUILD_DIR=$(mktemp -d)
    echo "Using build directory: $BUILD_DIR"
    
    # Copy PKGBUILD to temp directory
    cp PKGBUILD "$BUILD_DIR/"
    cd "$BUILD_DIR"
    
    # Build the package
    makepkg -si --noconfirm
    
    echo "✓ XFB built and installed"
    echo ""
    
    # Cleanup
    cd -
    rm -rf "$BUILD_DIR"
}

# Verify installation
verify_installation() {
    echo "Step 5: Verifying installation..."
    
    if command -v xfb &> /dev/null; then
        echo "✓ XFB executable found in PATH"
        XFB_VERSION=$(xfb --version 2>&1 || echo "Version check not available")
        echo "  Version info: $XFB_VERSION"
    else
        echo "❌ XFB executable not found in PATH"
        return 1
    fi
    
    if [ -f "/usr/share/applications/xfb.desktop" ]; then
        echo "✓ Desktop file installed"
    else
        echo "⚠ Desktop file not found"
    fi
    
    if [ -f "/usr/share/pixmaps/xfb.png" ]; then
        echo "✓ Icon installed"
    else
        echo "⚠ Icon not found"
    fi
    
    echo ""
}

# Test accessibility features
test_accessibility() {
    echo "Step 6: Testing accessibility features..."
    
    if systemctl --user is-active --quiet at-spi-dbus-bus.service; then
        echo "✓ AT-SPI D-Bus service is running"
    else
        echo "⚠ AT-SPI D-Bus service not running (may need to start manually)"
    fi
    
    if command -v orca &> /dev/null; then
        echo "✓ ORCA screen reader is installed"
    else
        echo "⚠ ORCA not installed (optional)"
    fi
    
    if systemctl --user is-active --quiet speech-dispatcher.service 2>/dev/null; then
        echo "✓ Speech Dispatcher is running"
    else
        echo "⚠ Speech Dispatcher not running (may need to start manually)"
    fi
    
    echo ""
}

# Main execution
main() {
    check_environment
    
    echo "This script will:"
    echo "  1. Update your system"
    echo "  2. Install required dependencies"
    echo "  3. Install optional dependencies"
    echo "  4. Build and install XFB from PKGBUILD"
    echo "  5. Verify the installation"
    echo "  6. Test accessibility features"
    echo ""
    
    read -p "Continue? (y/N) " -n 1 -r
    echo ""
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled"
        exit 0
    fi
    
    update_system
    install_dependencies
    install_optional_deps
    build_and_install
    verify_installation
    test_accessibility
    
    echo "=========================================="
    echo "✓ Installation test completed!"
    echo "=========================================="
    echo ""
    echo "To launch XFB, run: xfb"
    echo ""
    echo "For accessibility features:"
    echo "  - Start ORCA: orca &"
    echo "  - Start Speech Dispatcher: systemctl --user start speech-dispatcher"
    echo ""
    echo "To uninstall: sudo pacman -R xfb"
}

main "$@"
