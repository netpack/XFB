#!/bin/bash
# Update the Homebrew tap (netpack/homebrew-xfb) so users can install XFB with:
#   brew install --cask netpack/xfb/xfb
#
# What this script does (each push step asks for confirmation):
#   1. Computes the sha256 of the freshly built XFB-<version>-macOS.dmg
#   2. Regenerates the cask from packaging/homebrew/xfb.rb
#   3. Uploads the dmg to the GitHub release v<version> (via gh)
#   4. Commits and pushes the cask to the tap repository
#
# Prerequisites: ./build-macos.sh has produced the dmg, `gh` is authenticated,
# and the release tag v<version> exists (create it with release.sh / gh).

set -e

VERSION="${1:-3.14159265}"
DMG="XFB-${VERSION}-macOS.dmg"
TAP_REPO="netpack/homebrew-xfb"
TAP_DIR="homebrew-xfb"
CASK_TEMPLATE="packaging/homebrew/xfb.rb"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_status()  { echo -e "${GREEN}✓${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_error()   { echo -e "${RED}✗${NC} $1"; }

echo "═══ XFB Homebrew tap update — version ${VERSION} ═══"
echo ""

# --- Pre-flight -------------------------------------------------------------
if [ ! -f "$DMG" ]; then
    print_error "$DMG not found. Build it first with ./build-macos.sh"
    exit 1
fi
if ! command -v gh >/dev/null 2>&1; then
    print_error "GitHub CLI (gh) not found — needed to upload the dmg and manage the tap."
    exit 1
fi
if [ ! -f "$CASK_TEMPLATE" ]; then
    print_error "Cask template $CASK_TEMPLATE not found."
    exit 1
fi

SHA256=$(shasum -a 256 "$DMG" | awk '{print $1}')
print_status "dmg sha256: $SHA256"

# --- Upload the dmg to the GitHub release -----------------------------------
echo ""
if gh release view "v${VERSION}" --repo netpack/XFB >/dev/null 2>&1; then
    print_status "Release v${VERSION} exists on GitHub"
else
    print_warning "Release v${VERSION} does not exist on github.com/netpack/XFB yet."
    read -p "Create it now as a draft? (y/n) " -n 1 -r; echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        gh release create "v${VERSION}" --repo netpack/XFB --draft \
            --title "XFB ${VERSION}" --notes "XFB ${VERSION}"
        print_status "Draft release v${VERSION} created (publish it from the GitHub UI)"
    else
        print_error "Cannot continue without the release."
        exit 1
    fi
fi

read -p "Upload ${DMG} to release v${VERSION}? (y/n) " -n 1 -r; echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    gh release upload "v${VERSION}" "$DMG" --repo netpack/XFB --clobber
    print_status "dmg uploaded"
else
    print_warning "Skipped dmg upload — the cask URL will 404 until it is uploaded!"
fi

# --- Prepare the tap repository ----------------------------------------------
echo ""
if [ ! -d "$TAP_DIR" ]; then
    if gh repo view "$TAP_REPO" >/dev/null 2>&1; then
        git clone "https://github.com/${TAP_REPO}.git" "$TAP_DIR"
        print_status "Tap repository cloned"
    else
        print_warning "Tap repository ${TAP_REPO} does not exist yet."
        read -p "Create it now (public)? (y/n) " -n 1 -r; echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            gh repo create "$TAP_REPO" --public \
                --description "Homebrew tap for XFB radio automation software"
            git clone "https://github.com/${TAP_REPO}.git" "$TAP_DIR"
            print_status "Tap repository created and cloned"
        else
            print_error "Cannot continue without the tap repository."
            exit 1
        fi
    fi
else
    (cd "$TAP_DIR" && git pull)
    print_status "Tap repository updated"
fi

# --- Generate the cask --------------------------------------------------------
mkdir -p "$TAP_DIR/Casks"
sed -e "s/^  version \".*\"/  version \"${VERSION}\"/" \
    -e "s/^  sha256 \".*\"/  sha256 \"${SHA256}\"/" \
    "$CASK_TEMPLATE" > "$TAP_DIR/Casks/xfb.rb"
print_status "Cask generated: $TAP_DIR/Casks/xfb.rb"

# Keep the in-repo template's version in sync (sha stays a placeholder there)
sed -i '' -e "s/^  version \".*\"/  version \"${VERSION}\"/" "$CASK_TEMPLATE" 2>/dev/null || \
    sed -i -e "s/^  version \".*\"/  version \"${VERSION}\"/" "$CASK_TEMPLATE"

# --- Commit & push -------------------------------------------------------------
cd "$TAP_DIR"
git add Casks/xfb.rb
if git diff --cached --quiet; then
    print_status "Cask unchanged — nothing to push."
    exit 0
fi

echo ""
git diff --cached
echo ""
read -p "Push the cask update to ${TAP_REPO}? (y/n) " -n 1 -r; echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git commit -m "xfb ${VERSION}"
    git push
    print_status "Tap updated! Users can now run:"
    echo "    brew install --cask netpack/xfb/xfb"
else
    print_warning "Push cancelled. Push manually later with: cd ${TAP_DIR} && git commit && git push"
fi
