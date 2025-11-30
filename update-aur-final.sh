#!/bin/bash
set -e

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          Updating XFB AUR to v2.0.0-5                    ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Clone AUR repo if not exists
if [ ! -d "aur-xfb" ]; then
    echo "Cloning AUR repository..."
    git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
else
    echo "AUR repository already exists, updating..."
    cd aur-xfb
    git pull
    cd ..
fi

cd aur-xfb

# Copy new PKGBUILD
echo "Copying updated PKGBUILD..."
cp ../PKGBUILD .

# Generate .SRCINFO
echo "Generating .SRCINFO..."
makepkg --printsrcinfo > .SRCINFO

# Show changes
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "Changes to be committed:"
echo "═══════════════════════════════════════════════════════════"
git diff PKGBUILD .SRCINFO

echo ""
echo "═══════════════════════════════════════════════════════════"
read -p "Commit and push to AUR? (y/N) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    git add PKGBUILD .SRCINFO
    git commit -m "Update to version 2.0.0-5

- Fixed linking errors in unit and integration tests
- Added missing accessibility service dependencies
- Removed references to disabled services in test builds
- Build now completes successfully on all architectures
"
    
    echo ""
    echo "Pushing to AUR..."
    git push
    
    echo ""
    echo "╔══════════════════════════════════════════════════════════╗"
    echo "║          ✅ AUR Package Updated Successfully!            ║"
    echo "╚══════════════════════════════════════════════════════════╝"
    echo ""
    echo "Verify at: https://aur.archlinux.org/packages/xfb"
    echo ""
    echo "Users can now install with:"
    echo "  yay -S xfb"
    echo ""
    echo "The package will show version 2.0.0-5 within 5 minutes."
else
    echo "Update cancelled"
fi

cd ..
