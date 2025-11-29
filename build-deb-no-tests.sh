#!/bin/bash
# XFB - Debian Package Build Script (No Tests)
# This version skips test compilation to focus on packaging

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="XFB Radio Automation Software"
PACKAGE_NAME="xfb"
VERSION="2.0.0"
BUILD_DIR="build-package"
INSTALL_DIR="install"

# Detect architecture
ARCH=$(dpkg --print-architecture)

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  $PROJECT_NAME Build Script (No Tests)${NC}"
echo -e "${BLUE}========================================${NC}"

# Function to print status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're on a supported system
if ! command -v lsb_release >/dev/null 2>&1; then
    print_error "This script requires lsb_release. Please install lsb-release package."
    exit 1
fi

OS_ID=$(lsb_release -si)
OS_RELEASE=$(lsb_release -sr)
OS_CODENAME=$(lsb_release -sc)

print_status "Detected OS: $OS_ID $OS_RELEASE ($OS_CODENAME)"

# Verify this is Ubuntu or Debian
if [[ "$OS_ID" != "Ubuntu" && "$OS_ID" != "Debian" ]]; then
    print_warning "This script is designed for Ubuntu/Debian. Proceeding anyway..."
fi

# Check for required build dependencies
print_status "Checking build dependencies..."

REQUIRED_PACKAGES=(
    "build-essential"
    "cmake"
    "pkg-config"
    "qt6-base-dev"
    "qt6-multimedia-dev"
    "qt6-webengine-dev"
    "libatspi2.0-dev"
    "libspeechd-dev"
    "libasound2-dev"
    "libpulse-dev"
    "libsqlite3-dev"
    "libcurl4-openssl-dev"
)

MISSING_PACKAGES=()

for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -l "$package" >/dev/null 2>&1; then
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
    print_error "Missing required packages: ${MISSING_PACKAGES[*]}"
    echo "Please install them with:"
    echo "sudo apt update && sudo apt install ${MISSING_PACKAGES[*]}"
    exit 1
fi

print_status "All build dependencies satisfied."

# Clean previous builds
if [ -d "$BUILD_DIR" ]; then
    print_status "Cleaning previous build directory..."
    rm -rf "$BUILD_DIR"
fi

if [ -d "$INSTALL_DIR" ]; then
    print_status "Cleaning previous install directory..."
    rm -rf "$INSTALL_DIR"
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake (disable tests)
print_status "Configuring build with CMake (tests disabled)..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_CXX_FLAGS="-O2 -g -DNDEBUG" \
    -DCPACK_GENERATOR=DEB \
    -DCPACK_PACKAGE_VERSION="$VERSION" \
    -DCPACK_PACKAGE_CONTACT="Netpack <info@netpack.pt>" \
    -DCPACK_DEBIAN_PACKAGE_MAINTAINER="Netpack <info@netpack.pt>" \
    -DBUILD_TESTING=OFF

# Build only the main application
print_status "Building XFB (main application only)..."
make -j$(nproc) XFB

# Install to staging directory
print_status "Installing to staging directory..."
make install DESTDIR="../$INSTALL_DIR"

# Create additional directories and files for the package
print_status "Preparing package structure..."

# Create documentation directory
mkdir -p "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME"
cp -r ../docs/accessibility/* "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME/"

# Copy main documentation
if [ -f "../README.md" ]; then
    cp ../README.md "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME/"
fi
cp ../debian/copyright "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME/"
cp ../debian/changelog "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME/"

# Compress changelog
gzip -9 "../$INSTALL_DIR/usr/share/doc/$PACKAGE_NAME/changelog"

# Create man page directory and install man page
mkdir -p "../$INSTALL_DIR/usr/share/man/man1"
cat > "../$INSTALL_DIR/usr/share/man/man1/xfb.1" << 'EOF'
.TH XFB 1 "October 2025" "XFB 2.0.0" "User Commands"
.SH NAME
xfb \- XFB Radio Automation Software
.SH SYNOPSIS
.B xfb
[\fIOPTION\fR]...
.SH DESCRIPTION
XFB Radio Automation Software is designed for radio stations and broadcasting professionals. The software provides comprehensive accessibility support for visually impaired users through ORCA screen reader integration, keyboard navigation, audio feedback, and braille display support.
.SH OPTIONS
.TP
.B \-\-accessibility\-mode
Launch XFB with enhanced accessibility features enabled
.TP
.B \-\-help
Display help information and exit
.TP
.B \-\-version
Display version information and exit
.TP
.B \-\-config\-dir DIR
Use DIR as configuration directory
.TP
.B \-\-log\-level LEVEL
Set logging level (debug, info, warning, error)
.SH FILES
.TP
.I ~/.config/xfb/
User configuration directory
.TP
.I /etc/xfb/
System-wide configuration directory
.TP
.I /var/log/xfb/
Log file directory
.SH ACCESSIBILITY
XFB provides comprehensive accessibility support including:
.IP \(bu 2
Full ORCA screen reader compatibility
.IP \(bu 2
Complete keyboard navigation
.IP \(bu 2
Audio feedback for all actions
.IP \(bu 2
Braille display support via BrlTTY
.IP \(bu 2
WCAG 2.1 AA compliance
.SH EXAMPLES
.TP
Launch XFB with accessibility mode:
.B xfb \-\-accessibility\-mode
.TP
Launch with debug logging:
.B xfb \-\-log\-level debug
.SH SEE ALSO
.BR orca (1),
.BR brltty (1),
.BR speech-dispatcher (1)
.SH BUGS
Report bugs to <info@netpack.pt> or visit https://netpack.pt/
.SH AUTHOR
Netpack <info@netpack.pt>
EOF

# Compress man page
gzip -9 "../$INSTALL_DIR/usr/share/man/man1/xfb.1"

# Create package using CPack
print_status "Creating Debian package..."
cpack -G DEB

# Find the generated .deb file
DEB_FILE=$(find . -name "*.deb" | head -1)

if [ -z "$DEB_FILE" ]; then
    print_error "Failed to create .deb package!"
    exit 1
fi

# Move the .deb file to the project root with architecture-specific name
NEW_DEB_NAME="${PACKAGE_NAME}_${VERSION}-1_${ARCH}.deb"
mv "$DEB_FILE" "../$NEW_DEB_NAME"

cd ..

print_status "Package created successfully: $NEW_DEB_NAME"

# Display package information
print_status "Package information:"
dpkg-deb --info "$NEW_DEB_NAME"

echo ""
print_status "Package contents:"
dpkg-deb --contents "$NEW_DEB_NAME" | head -20
if [ $(dpkg-deb --contents "$NEW_DEB_NAME" | wc -l) -gt 20 ]; then
    echo "... (showing first 20 files, $(dpkg-deb --contents "$NEW_DEB_NAME" | wc -l) total)"
fi

# Verify package integrity
print_status "Verifying package integrity..."
if dpkg-deb --fsys-tarfile "$NEW_DEB_NAME" >/dev/null 2>&1; then
    print_status "Package integrity check passed."
else
    print_error "Package integrity check failed!"
    exit 1
fi

# Create installation instructions
cat > "INSTALL_INSTRUCTIONS.txt" << EOF
XFB Radio Automation Software - Installation Instructions
==================================================

Package: $NEW_DEB_NAME
Version: $VERSION
Built on: $(date)
System: $OS_ID $OS_RELEASE ($OS_CODENAME)

INSTALLATION:
============

1. Install the package:
   sudo dpkg -i $NEW_DEB_NAME

2. If there are dependency issues, fix them with:
   sudo apt-get install -f

3. Ensure ORCA is installed and running:
   sudo apt install orca
   orca --replace &

4. Launch XFB with accessibility mode:
   xfb --accessibility-mode

SYSTEM REQUIREMENTS:
==================

- Ubuntu 20.04 LTS or newer (recommended: 22.04 LTS)
- ORCA screen reader 40.0 or newer
- 4 GB RAM minimum (16 GB recommended)
- 2 GB disk space minimum

ACCESSIBILITY FEATURES:
=====================

- Full ORCA screen reader integration
- Complete keyboard navigation (no mouse required)
- Audio feedback for all user actions
- Braille display support via BrlTTY
- Live region updates for dynamic content
- Customizable accessibility preferences
- WCAG 2.1 AA compliance

GETTING STARTED:
===============

1. Launch XFB from the Applications menu or command line
2. Use Tab/Shift+Tab to navigate between controls
3. Use arrow keys for list and table navigation
4. Press F1 for context-sensitive help
5. Access preferences with Ctrl+P

KEYBOARD SHORTCUTS:
==================

- Ctrl+P: Accessibility Preferences
- Ctrl+H: Help System
- F1: Context Help
- Ctrl+Q: Quit Application
- Space: Play/Pause
- Ctrl+O: Open File
- Ctrl+S: Save

SUPPORT:
========

- Documentation: /usr/share/doc/xfb/
- Online Help: https://netpack.pt/
- Email Support: info@netpack.pt
- Bug Reports: Use Ctrl+Shift+B in XFB

TROUBLESHOOTING:
===============

If XFB doesn't speak with ORCA:
1. Ensure AT-SPI is enabled: gsettings set org.gnome.desktop.interface toolkit-accessibility true
2. Restart ORCA: orca --replace &
3. Launch XFB with: xfb --accessibility-mode

For audio issues:
1. Check PulseAudio: pulseaudio --check
2. Test audio: speaker-test -c 2
3. Check permissions: groups \$USER (should include 'audio')

For more help, see the troubleshooting guide:
/usr/share/doc/xfb/user-guide/troubleshooting.md
EOF

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  BUILD COMPLETED SUCCESSFULLY!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${BLUE}Package:${NC} $NEW_DEB_NAME"
echo -e "${BLUE}Size:${NC} $(du -h "$NEW_DEB_NAME" | cut -f1)"
echo -e "${BLUE}Installation Instructions:${NC} INSTALL_INSTRUCTIONS.txt"
echo ""
echo -e "${YELLOW}Next Steps:${NC}"
echo "1. Test the package on a clean Ubuntu system"
echo "2. Send $NEW_DEB_NAME to your beta tester"
echo "3. Include INSTALL_INSTRUCTIONS.txt for setup guidance"
echo ""
echo -e "${GREEN}Ready for beta testing!${NC}"