#!/bin/bash

# Script to create a .deb package for XFB on Debian Linux

set -e

PKG_NAME="xfb"
PKG_VERSION="1.23"
MAINTAINER="Frédéric Bogaerts <your-email@example.com>"
ARCH="amd64"  # Adjust as needed
DESCRIPTION="XFB - Radio automation software"
DEPENDENCIES=""

# Paths - adjusted BINARY_PATH to your built Linux binary location
BINARY_PATH="./build/Qt_6_7_3_for_Linux-Release/xfb"  # Adjust this path if different
INSTALL_PREFIX="/usr/local"
BIN_DIR="$INSTALL_PREFIX/bin"
DESKTOP_DIR="/usr/share/applications"
ICON_DIR="/usr/share/icons/hicolor/48x48/apps"

# Temporary packaging directory
PKG_DIR="xfb_deb_pkg"

echo "Creating Debian package structure..."

rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR$BIN_DIR"
mkdir -p "$PKG_DIR$DESKTOP_DIR"
mkdir -p "$PKG_DIR$ICON_DIR"

# Copy binary
if [ ! -f "$BINARY_PATH" ]; then
    echo "Error: Binary not found at $BINARY_PATH"
    exit 1
fi
cp "$BINARY_PATH" "$PKG_DIR$BIN_DIR/xfb"
chmod 755 "$PKG_DIR$BIN_DIR/xfb"

# Create a simple desktop entry
cat > "$PKG_DIR$DESKTOP_DIR/xfb.desktop" <<EOF
[Desktop Entry]
Name=XFB
Comment=Radio automation software
Exec=$BIN_DIR/xfb
Icon=xfb
Terminal=false
Type=Application
Categories=AudioVideo;Audio;Player;
EOF

# Copy icon - adjust path if you have a Linux icon
if [ -f "./src/icons/48x48.png" ]; then
    cp "./src/icons/48x48.png" "$PKG_DIR$ICON_DIR/xfb.png"
fi

# Create DEBIAN control file
mkdir -p "$PKG_DIR/DEBIAN"
cat > "$PKG_DIR/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $PKG_VERSION
Section: sound
Priority: optional
Architecture: $ARCH
Depends: $DEPENDENCIES
Maintainer: $MAINTAINER
Description: $DESCRIPTION
EOF

echo "Building .deb package..."
dpkg-deb --build "$PKG_DIR"

echo "Package created: ${PKG_DIR}.deb"
