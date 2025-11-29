#!/bin/bash
# Simple one-command AUR update script
# Run this on an Arch Linux system

set -e

echo "XFB AUR Quick Update"
echo "===================="
echo ""

# Check if on Arch
if ! command -v makepkg &> /dev/null; then
    echo "❌ Error: makepkg not found. This must be run on Arch Linux."
    echo ""
    echo "Options:"
    echo "1. Run this on an Arch Linux system"
    echo "2. Use Docker: docker run -it archlinux bash"
    echo "3. See UPDATE_AUR_NOW.md for detailed instructions"
    exit 1
fi

# Clone or update AUR repo
if [ -d "aur-xfb" ]; then
    echo "Updating existing AUR clone..."
    cd aur-xfb
    git pull
else
    echo "Cloning AUR repository..."
    git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
    cd aur-xfb
fi

# Copy PKGBUILD
echo "Copying PKGBUILD..."
cp ../PKGBUILD .

# Generate .SRCINFO
echo "Generating .SRCINFO..."
makepkg --printsrcinfo > .SRCINFO

# Show changes
echo ""
echo "Changes:"
git diff PKGBUILD .SRCINFO

# Commit
echo ""
read -p "Commit and push? (y/N) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git add PKGBUILD .SRCINFO
    git commit -m "Update to version 2.0.0

- Updated to XFB 2.0.0
- Enhanced accessibility features with ORCA integration
- Improved keyboard navigation
- Added braille display support
- Performance optimizations
- Updated dependencies
- New CMake-based build system
"
    git push
    echo ""
    echo "✅ AUR package updated!"
    echo "Verify at: https://aur.archlinux.org/packages/xfb"
else
    echo "Cancelled"
fi
