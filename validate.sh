#!/bin/bash
# This script validates that all files are in place, sets permissions,
# finalizes them, and builds the Debian package.
# It does NOT copy or remove any files from the staging directory.

set -e # Exit immediately if a command fails

echo "--- STARTING DEBIAN PACKAGE VALIDATION AND BUILD ---"

# --- Configuration ---
STAGING_DIR="src/build/deb/xfb"
OUTPUT_DIR="src/build/deb"

# --- 1. Validate File Existence ---
echo "Validating that all required files exist..."

# Helper function for checking files
check_file() {
    if [ ! -f "$1" ]; then
        echo "ERROR: Missing required file: $1"
        exit 1
    fi
    echo "  [OK] Found $1"
}

# Check all critical files
check_file "$STAGING_DIR/DEBIAN/control"
check_file "$STAGING_DIR/DEBIAN/postinst"
check_file "$STAGING_DIR/DEBIAN/postrm"
check_file "$STAGING_DIR/usr/bin/XFB"
check_file "$STAGING_DIR/usr/share/applications/xfb.desktop"
check_file "$STAGING_DIR/usr/share/doc/xfb/copyright"
check_file "$STAGING_DIR/usr/share/doc/xfb/changelog" # Checks for the uncompressed version
check_file "$STAGING_DIR/usr/share/icons/hicolor/256x256/apps/xfb.png"
check_file "$STAGING_DIR/usr/share/xfb/config/xfb.conf"

echo "All required files are present."

# --- 2. Set Permissions and Finalize ---
echo "Setting permissions and finalizing files..."

# Set executable permissions
chmod 755 "$STAGING_DIR/usr/bin/XFB"
chmod 755 "$STAGING_DIR/DEBIAN/postinst"
chmod 755 "$STAGING_DIR/DEBIAN/postrm"

# Set read-only permissions for all data files
find "$STAGING_DIR/usr/share" -type f -exec chmod 644 {} \;

# Strip debugging symbols from the binary
echo "Stripping binary..."
strip "$STAGING_DIR/usr/bin/XFB"

# Compress the changelog with correct options
echo "Compressing changelog..."
# Remove any old gzipped file first to be safe
rm -f "$STAGING_DIR/usr/share/doc/xfb/changelog.gz"
gzip -n -9 "$STAGING_DIR/usr/share/doc/xfb/changelog"

echo "Preparation complete."

# --- 3. Build the Package ---
echo "Building the .deb package..."
fakeroot dpkg-deb --build "$STAGING_DIR" "$HOME/xfb_1-23.deb"

# --- 4. Run Lintian ---
echo "Running lintian check..."
lintian "$HOME/xfb_1-23.deb"

echo "--- BUILD COMPLETE ---"
echo "Package is ready at: $HOME/xfb_1-23.deb"
