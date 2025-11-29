#!/bin/bash
# Master release script for XFB
# Coordinates GitHub, AUR, and Debian repository updates

set -e

VERSION="2.0.0"
RELEASE_NAME="XFB $VERSION - Accessibility Enhanced"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          XFB Release Automation Script                  ║"
echo "║                  Version $VERSION                         ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Pre-flight checks
echo "Running pre-flight checks..."
echo ""

CHECKS_PASSED=true

# Check for required tools
if ! command_exists git; then
    print_error "git not found"
    CHECKS_PASSED=false
else
    print_status "git found"
fi

if ! command_exists cmake; then
    print_warning "cmake not found (needed for building)"
fi

if ! command_exists dpkg-deb; then
    print_warning "dpkg-deb not found (needed for .deb packages)"
fi

if ! command_exists makepkg; then
    print_warning "makepkg not found (needed for AUR)"
fi

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    print_error "Not in a git repository"
    CHECKS_PASSED=false
else
    print_status "Git repository detected"
fi

# Check for required files
REQUIRED_FILES=(
    "PKGBUILD"
    "CMakeLists.txt"
    "build-deb-no-tests.sh"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        print_error "Required file missing: $file"
        CHECKS_PASSED=false
    fi
done

if [ "$CHECKS_PASSED" = false ]; then
    echo ""
    print_error "Pre-flight checks failed. Please fix the issues above."
    exit 1
fi

print_status "All pre-flight checks passed"
echo ""

# Main menu
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                    Release Options                       ║"
echo "╠══════════════════════════════════════════════════════════╣"
echo "║  1. Full Release (All platforms)                         ║"
echo "║  2. GitHub Only                                          ║"
echo "║  3. AUR Only                                             ║"
echo "║  4. Debian Repository Only                               ║"
echo "║  5. Build Packages Only (no upload)                      ║"
echo "║  6. Safety Check Only                                    ║"
echo "║  0. Exit                                                 ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

read -p "Select option: " OPTION

case $OPTION in
    1)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  Full Release Process"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        
        # Step 1: Safety check
        echo "Step 1/5: Running safety checks..."
        ./push-to-github.sh
        
        # Step 2: Build packages
        echo ""
        echo "Step 2/5: Building .deb package..."
        if [ -f "build-deb-no-tests.sh" ]; then
            ./build-deb-no-tests.sh
            print_status ".deb package built"
        else
            print_warning "Skipping .deb build (script not found)"
        fi
        
        # Step 3: Update AUR
        echo ""
        echo "Step 3/5: Updating AUR..."
        read -p "Update AUR package? (y/N) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            ./update-aur.sh
        else
            print_warning "Skipped AUR update"
        fi
        
        # Step 4: Update Debian repository
        echo ""
        echo "Step 4/5: Updating Debian repository..."
        read -p "Update Debian repository? (y/N) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            ./update-debian-repo.sh
        else
            print_warning "Skipped Debian repository update"
        fi
        
        # Step 5: Create GitHub release
        echo ""
        echo "Step 5/5: GitHub Release..."
        echo ""
        echo "To create a GitHub release:"
        echo "1. Go to https://github.com/netpack/XFB/releases/new"
        echo "2. Tag: v$VERSION"
        echo "3. Title: $RELEASE_NAME"
        echo "4. Upload: xfb_${VERSION}-1_amd64.deb"
        echo "5. Upload: XFB-${VERSION}-macOS.dmg (if available)"
        echo ""
        read -p "Press Enter when done..."
        
        echo ""
        print_status "Full release process completed!"
        ;;
        
    2)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  GitHub Push"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        ./push-to-github.sh
        ;;
        
    3)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  AUR Update"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        ./update-aur.sh
        ;;
        
    4)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  Debian Repository Update"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        ./update-debian-repo.sh
        ;;
        
    5)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  Build Packages"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        
        echo "Building .deb package..."
        if [ -f "build-deb-no-tests.sh" ]; then
            ./build-deb-no-tests.sh
            print_status ".deb package built"
        fi
        
        echo ""
        echo "Building Arch package..."
        if command_exists makepkg; then
            makepkg -sf
            print_status "Arch package built"
        else
            print_warning "makepkg not available (install on Arch Linux)"
        fi
        ;;
        
    6)
        echo ""
        echo "═══════════════════════════════════════════════════════════"
        echo "  Safety Check"
        echo "═══════════════════════════════════════════════════════════"
        echo ""
        
        # Run safety checks without pushing
        echo "Checking for sensitive information..."
        
        if git grep -i "password\s*=\s*['\"][^'\"]*['\"]" -- '*.cpp' '*.h' 2>/dev/null | grep -v "Password:" | grep -v "//"; then
            print_warning "Possible hardcoded password found"
        else
            print_status "No hardcoded passwords"
        fi
        
        if git grep -l "BEGIN.*PRIVATE KEY" 2>/dev/null; then
            print_warning "Private key found"
        else
            print_status "No private keys"
        fi
        
        if [ -f "config/.netrc" ]; then
            if grep -v "\[" config/.netrc | grep -qE "password\s+[^[]"; then
                print_warning "Real credentials in .netrc"
            else
                print_status ".netrc contains only placeholders"
            fi
        fi
        
        print_status "Safety check completed"
        ;;
        
    0)
        echo "Exiting..."
        exit 0
        ;;
        
    *)
        print_error "Invalid option"
        exit 1
        ;;
esac

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                    Release Summary                       ║"
echo "╠══════════════════════════════════════════════════════════╣"
echo "║  Version: $VERSION                                        ║"
echo "║  GitHub: https://github.com/netpack/XFB                  ║"
echo "║  AUR: https://aur.archlinux.org/packages/xfb             ║"
echo "║  Docs: https://netpack.pt                                ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""
