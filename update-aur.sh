#!/bin/bash
# Script to update XFB package on AUR
# Usage: ./update-aur.sh

set -e

VERSION="3.141592"
PKGREL="1"

echo "=========================================="
echo "Updating XFB on AUR to version $VERSION"
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "PKGBUILD" ]; then
    echo "❌ Error: PKGBUILD not found. Run this script from the XFB root directory."
    exit 1
fi

# Check if AUR repo exists
if [ ! -d "aur-xfb" ]; then
    echo "Cloning existing AUR repository..."
    git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
    echo "✓ AUR repository cloned"
else
    echo "✓ AUR repository already exists"
    cd aur-xfb
    git pull
    cd ..
fi

echo ""
echo "Copying PKGBUILD and install script to AUR directory..."
cp PKGBUILD xfb.install aur-xfb/

cd aur-xfb

echo "Generating .SRCINFO..."
if command -v makepkg &>/dev/null; then
    makepkg --printsrcinfo > .SRCINFO
else
    # makepkg doesn't exist on macOS: generate .SRCINFO in an Arch container.
    # makepkg refuses to run as root, and xfb.install must sit next to the
    # PKGBUILD (install=xfb.install), hence the builder user and the copies.
    echo "makepkg not found — generating .SRCINFO via Docker (archlinux)..."
    docker run --rm --platform linux/amd64 -v "$PWD":/pkg archlinux:latest \
        bash -c "useradd -m builder && install -m644 -o builder /pkg/PKGBUILD /pkg/xfb.install /home/builder/ && cd /home/builder && su builder -c 'makepkg --printsrcinfo' > /pkg/.SRCINFO"
fi

echo ""
echo "Changes to be committed:"
git diff PKGBUILD .SRCINFO

echo ""
read -p "Review the changes above. Continue with commit? (y/N) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Update cancelled"
    exit 0
fi

echo ""
echo "Committing changes..."
git add PKGBUILD .SRCINFO xfb.install
git commit -m "Update to version $VERSION-$PKGREL

- Updated to XFB 3.141592
- Download full SoundCloud sets in the external downloader
- Options -> Install all dependencies (one-click external tool setup)
- Transparent application icon (hicolor + scalable SVG)
"

echo ""
read -p "Push to AUR? (y/N) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Pushing to AUR..."
    git push
    echo ""
    echo "=========================================="
    echo "✓ Successfully updated AUR package!"
    echo "=========================================="
    echo ""
    echo "View at: https://aur.archlinux.org/packages/xfb"
    echo ""
else
    echo "Push cancelled. You can push manually later with:"
    echo "  cd aur-xfb && git push"
fi

cd ..
