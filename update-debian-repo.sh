#!/bin/bash
# Script to update Debian repository on GitHub Pages
# Usage: ./update-debian-repo.sh

set -e

VERSION="2.0.0"
ARCH="amd64"
DEB_FILE="xfb_${VERSION}-1_${ARCH}.deb"

echo "=========================================="
echo "Updating Debian Repository"
echo "=========================================="
echo ""

# Check if .deb file exists
if [ ! -f "$DEB_FILE" ]; then
    echo "❌ Error: $DEB_FILE not found"
    echo "Run ./build-deb-no-tests.sh first"
    exit 1
fi

echo "✓ Found $DEB_FILE"
echo ""

# Create repository structure
echo "Creating repository structure..."
mkdir -p debian-repo/pool/main
mkdir -p debian-repo/dists/stable/main/binary-amd64
mkdir -p debian-repo/dists/stable/main/binary-arm64

# Copy .deb file
echo "Copying package to repository..."
cp "$DEB_FILE" debian-repo/pool/main/

# Check if arm64 package exists
ARM64_DEB="xfb_${VERSION}-1_arm64.deb"
if [ -f "$ARM64_DEB" ]; then
    echo "✓ Found ARM64 package"
    cp "$ARM64_DEB" debian-repo/pool/main/
fi

cd debian-repo

# Generate Packages files
echo "Generating Packages files..."

# AMD64
dpkg-scanpackages --arch amd64 pool/main /dev/null | gzip -9c > dists/stable/main/binary-amd64/Packages.gz
dpkg-scanpackages --arch amd64 pool/main /dev/null > dists/stable/main/binary-amd64/Packages

# ARM64 (if exists)
if [ -f "pool/main/$ARM64_DEB" ]; then
    dpkg-scanpackages --arch arm64 pool/main /dev/null | gzip -9c > dists/stable/main/binary-arm64/Packages.gz
    dpkg-scanpackages --arch arm64 pool/main /dev/null > dists/stable/main/binary-arm64/Packages
fi

# Generate Release file
echo "Generating Release file..."
cd dists/stable

ARCHITECTURES="amd64"
if [ -f "../../pool/main/$ARM64_DEB" ]; then
    ARCHITECTURES="amd64 arm64"
fi

cat > Release << EOF
Origin: XFB
Label: XFB Radio Automation
Suite: stable
Codename: stable
Version: 2.0
Architectures: $ARCHITECTURES
Components: main
Description: XFB Radio Automation Software - Professional radio broadcasting solution with comprehensive accessibility support
Date: $(date -R)
EOF

# Add file checksums
echo "MD5Sum:" >> Release
find . -type f -name "Packages*" -exec md5sum {} \; | sed 's/\.\///' >> Release

echo "SHA1:" >> Release
find . -type f -name "Packages*" -exec sha1sum {} \; | sed 's/\.\///' >> Release

echo "SHA256:" >> Release
find . -type f -name "Packages*" -exec sha256sum {} \; | sed 's/\.\///' >> Release

cd ../..

# Create README for repository
cat > README.md << 'EOF'
# XFB Debian Repository

This repository provides Debian packages for XFB Radio Automation Software.

## Installation

### Quick Install

```bash
# Add repository (without GPG verification)
echo "deb [trusted=yes] https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update package list
sudo apt update

# Install XFB
sudo apt install xfb
```

### Secure Install (with GPG verification)

```bash
# Add GPG key
wget -qO - https://netpack.github.io/XFB/xfb-archive-keyring.gpg | sudo apt-key add -

# Add repository
echo "deb https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update and install
sudo apt update
sudo apt install xfb
```

## Supported Distributions

- Debian 11 (Bullseye) and newer
- Ubuntu 20.04 LTS and newer
- Linux Mint 20 and newer
- Any Debian-based distribution with Qt6 support

## Supported Architectures

- amd64 (x86_64)
- arm64 (aarch64) - if available

## Package Information

- **Package Name**: xfb
- **Current Version**: 2.0.0
- **License**: GPL-3.0
- **Maintainer**: Netpack <info@netpack.pt>

## Manual Installation

If you prefer to download the .deb file directly:

```bash
# Download
wget https://github.com/netpack/XFB/releases/download/v2.0.0/xfb_2.0.0-1_amd64.deb

# Install
sudo apt install ./xfb_2.0.0-1_amd64.deb
```

## Updating

```bash
sudo apt update
sudo apt upgrade xfb
```

## Uninstalling

```bash
sudo apt remove xfb
```

## Support

- **Website**: https://netpack.pt
- **GitHub**: https://github.com/netpack/XFB
- **Issues**: https://github.com/netpack/XFB/issues
- **Email**: info@netpack.pt

## About XFB

XFB is a professional radio automation software designed for radio stations and broadcasting professionals. Features include:

- Professional audio playback and recording
- Comprehensive accessibility support (ORCA screen reader integration)
- Complete keyboard navigation
- Playlist management
- Live streaming integration
- Multi-format audio support
- Database-driven music library

For more information, visit https://netpack.pt
EOF

cd ..

echo ""
echo "✓ Repository structure created"
echo ""
echo "Repository contents:"
tree debian-repo/ 2>/dev/null || find debian-repo/ -type f

echo ""
echo "=========================================="
echo "Next Steps:"
echo "=========================================="
echo ""
echo "1. Review the repository structure in debian-repo/"
echo ""
echo "2. To publish on GitHub Pages:"
echo "   git checkout gh-pages || git checkout --orphan gh-pages"
echo "   git rm -rf . 2>/dev/null || true"
echo "   cp -r debian-repo/* ."
echo "   git add ."
echo "   git commit -m 'Update Debian repository to version $VERSION'"
echo "   git push origin gh-pages"
echo ""
echo "3. Enable GitHub Pages in repository settings"
echo "   Settings → Pages → Source: gh-pages branch"
echo ""
echo "4. Users can then install with:"
echo "   echo 'deb [trusted=yes] https://YOUR_USERNAME.github.io/XFB stable main' | sudo tee /etc/apt/sources.list.d/xfb.list"
echo "   sudo apt update && sudo apt install xfb"
echo ""
