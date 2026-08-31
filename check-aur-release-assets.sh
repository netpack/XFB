#!/bin/bash
# Refuse an AUR push that would ship a package nobody can build.
#
# Usage: ./check-aur-release-assets.sh [expected-version]
#
# Arch is the only channel that *downloads* the companion app from the GitHub
# release; every other package carries it (see the table in
# android/RELEASING.md). That leaves two release-time steps manual, and 3.1422
# shipped with both of them missed: the assets were never uploaded, and the
# PKGBUILD went out with its sha256 placeholders unsubstituted. Neither shows
# up until a user runs makepkg, so the first anyone heard of it was an AUR
# comment. Every script that pushes to the AUR calls this first.

set -e

if [ ! -f "PKGBUILD" ]; then
    echo "❌ PKGBUILD not found. Run this from the XFB root directory."
    exit 1
fi

PKGVER="$(sed -n 's/^pkgver=//p' PKGBUILD)"
VERSION="${1:-$PKGVER}"

if [ "$PKGVER" != "$VERSION" ]; then
    echo "❌ Version mismatch: PKGBUILD says pkgver=$PKGVER, caller says $VERSION"
    echo "   Whichever is stale, fix it before pushing."
    exit 1
fi

if grep -q 'REPLACE_WITH' PKGBUILD; then
    echo "❌ PKGBUILD still carries checksum placeholders:"
    grep -n 'REPLACE_WITH' PKGBUILD | sed 's/^/     /'
    echo ""
    echo "   Substitute the real sums first. package-companion.sh prints both,"
    echo "   or read them off the staged files:"
    echo "     shasum -a 256 packaging/companion/xfb-companion.apk"
    echo "     shasum -a 256 packaging/companion/xfb-companion.json"
    exit 1
fi

# The sums the PKGBUILD declares must match what the release actually serves.
# One check covers both failure modes: assets absent, and sums left over from
# the previous version. Order follows source=() -- first hex is the APK,
# second the sidecar; 'SKIP' has no hex and drops out on its own.
SUM_BLOCK="$(sed -n '/^sha256sums=/,/)/p' PKGBUILD | grep -o '[0-9a-f]\{64\}')"
APK_SUM="$(printf '%s\n' "$SUM_BLOCK" | sed -n 1p)"
JSON_SUM="$(printf '%s\n' "$SUM_BLOCK" | sed -n 2p)"

if ! command -v gh &>/dev/null; then
    echo "⚠  gh not found — cannot verify the companion assets on the release."
    read -p "   Push without that check? (y/N) " -n 1 -r
    echo ""
    [[ $REPLY =~ ^[Yy]$ ]] || exit 1
    exit 0
fi

echo "Verifying the companion assets on release v$VERSION..."
RELEASE_ASSETS="$(gh release view "v$VERSION" --repo netpack/XFB \
    --json assets -q '.assets[] | "\(.name) \(.digest)"' 2>/dev/null || true)"

if [ -z "$RELEASE_ASSETS" ]; then
    echo "❌ No release v$VERSION on GitHub (or gh is not logged in)."
    echo "   The PKGBUILD sources the companion app from that release, so the"
    echo "   tag has to exist and carry the assets before the AUR push."
    exit 1
fi

GUARD_FAILED=false
check_asset() {
    # $1 = asset name, $2 = sum the PKGBUILD declares
    local name="$1" want="$2" got
    got="$(printf '%s\n' "$RELEASE_ASSETS" | awk -v n="$name" \
        '$1 == n { sub(/^sha256:/, "", $2); print $2 }')"
    if [ -z "$got" ]; then
        echo "❌ $name is not on release v$VERSION"
        GUARD_FAILED=true
    elif [ "$got" != "$want" ]; then
        echo "❌ $name checksum mismatch"
        echo "     PKGBUILD: $want"
        echo "     release:  $got"
        GUARD_FAILED=true
    else
        echo "✓ $name matches the PKGBUILD"
    fi
}

check_asset "xfb-companion.apk"  "$APK_SUM"
check_asset "xfb-companion.json" "$JSON_SUM"

if [ "$GUARD_FAILED" = true ]; then
    echo ""
    echo "   Upload the staged companion files, then re-run:"
    echo "     gh release upload \"v$VERSION\" \\"
    echo "       packaging/companion/xfb-companion.apk \\"
    echo "       packaging/companion/xfb-companion.json \\"
    echo "       --repo netpack/XFB --clobber"
    exit 1
fi
