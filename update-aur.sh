#!/bin/bash
# Script to update XFB package on AUR
# Usage: ./update-aur.sh

set -e

VERSION="2.0.0"
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
echo "Copying PKGBUILD to AUR directory..."
cp PKGBUILD aur-xfb/

cd aur-xfb

echo "Generating .SRCINFO..."
makepkg --printsrcinfo > .SRCINFO

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
git add PKGBUILD .SRCINFO
git commit -m "Update to version $VERSION-$PKGREL

- Updated to XFB 2.0.0
- Enhanced accessibility features with ORCA integration
- Improved keyboard navigation
- Added braille display support
- Performance optimizations
- Updated dependencies
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
