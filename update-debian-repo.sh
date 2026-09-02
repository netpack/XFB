#!/bin/bash
# Build the APT repository tree for GitHub Pages.
#
# The index generation needs Debian tooling that does not exist on macOS
# (dpkg-scanpackages from dpkg-dev, plus GNU coreutils md5sum/sha1sum/
# sha256sum), so that part runs inside a Debian container — the same approach
# build-deb-docker.sh already uses for the packages themselves. The result is
# byte-for-byte what a Debian host would produce.
#
# Usage:
#   ./update-debian-repo.sh [version]
#
# Publishing to gh-pages is deliberately NOT automatic; see the notes printed
# at the end.

set -e

VERSION="${1:-3.1423}"
REPO_DIR="debian-repo"
HELPER_IMAGE="xfb-apt-repo:bookworm"

echo "=========================================="
echo "Building APT repository for XFB $VERSION"
echo "=========================================="
echo ""

if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Start Docker Desktop and try again."
    exit 1
fi

# --- Locate the packages -------------------------------------------------
# build-deb-docker.sh writes to output/; older scripts left them in the root.
find_deb() {
    local arch="$1" name="xfb_${VERSION}-1_$1.deb"
    for candidate in "output/$name" "$name"; do
        [ -f "$candidate" ] && { echo "$candidate"; return 0; }
    done
    return 1
}

AMD64_DEB="$(find_deb amd64 || true)"
ARM64_DEB="$(find_deb arm64 || true)"

if [ -z "$AMD64_DEB" ] && [ -z "$ARM64_DEB" ]; then
    echo "❌ No packages found for $VERSION."
    echo "   Expected output/xfb_${VERSION}-1_{amd64,arm64}.deb"
    echo "   Run: ./build-deb-docker.sh amd64 && ./build-deb-docker.sh arm64"
    exit 1
fi

ARCHITECTURES=""
[ -n "$AMD64_DEB" ] && { echo "✓ amd64: $AMD64_DEB"; ARCHITECTURES="amd64"; }
[ -n "$ARM64_DEB" ] && { echo "✓ arm64: $ARM64_DEB"; ARCHITECTURES="${ARCHITECTURES:+$ARCHITECTURES }arm64"; }
echo ""

# --- Assemble the pool (portable: just mkdir/cp) -------------------------
echo "Assembling repository tree..."
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR/pool/main"
for arch in $ARCHITECTURES; do
    mkdir -p "$REPO_DIR/dists/stable/main/binary-$arch"
done
[ -n "$AMD64_DEB" ] && cp "$AMD64_DEB" "$REPO_DIR/pool/main/"
[ -n "$ARM64_DEB" ] && cp "$ARM64_DEB" "$REPO_DIR/pool/main/"

# --- Helper image (cached after the first run) ---------------------------
if ! docker image inspect "$HELPER_IMAGE" > /dev/null 2>&1; then
    echo "Building helper image (first run only)..."
    docker build -q -t "$HELPER_IMAGE" - > /dev/null <<'DOCKERFILE'
FROM debian:bookworm
RUN apt-get update && apt-get install -y --no-install-recommends dpkg-dev \
    && rm -rf /var/lib/apt/lists/*
DOCKERFILE
fi

# --- Generate indexes and Release inside Debian --------------------------
echo "Generating Packages and Release (in Debian container)..."
docker run --rm \
    -v "$(pwd)/$REPO_DIR:/repo" \
    -e "ARCHITECTURES=$ARCHITECTURES" \
    -w /repo \
    "$HELPER_IMAGE" bash -euc '
        for arch in $ARCHITECTURES; do
            dpkg-scanpackages --arch "$arch" pool/main /dev/null \
                > "dists/stable/main/binary-$arch/Packages"
            gzip -9c "dists/stable/main/binary-$arch/Packages" \
                > "dists/stable/main/binary-$arch/Packages.gz"
        done

        cd dists/stable
        cat > Release <<EOF
Origin: XFB
Label: XFB Radio Automation
Suite: stable
Codename: stable
Version: 2.0
Architectures: $ARCHITECTURES
Components: main
Description: XFB Radio Automation Software - radio broadcasting with comprehensive accessibility support
Date: $(date -R -u)
EOF
        # Checksums are relative to the Release file, and apt rejects a
        # Release listing files it cannot match, so emit size + path exactly.
        emit() {
            echo "$1:"
            find . -type f -name "Packages*" | sed "s|^\./||" | sort | while read -r f; do
                printf " %s %16d %s\n" "$($2 "$f" | cut -d" " -f1)" "$(stat -c%s "$f")" "$f"
            done
        }
        emit MD5Sum md5sum   >> Release
        emit SHA1   sha1sum  >> Release
        emit SHA256 sha256sum >> Release
    '

# Files created inside the container are root-owned; hand them back.
docker run --rm -v "$(pwd)/$REPO_DIR:/repo" "$HELPER_IMAGE" \
    chown -R "$(id -u):$(id -g)" /repo

# --- Landing page --------------------------------------------------------
cat > "$REPO_DIR/README.md" <<EOF
# XFB Debian Repository

APT repository for XFB Radio Automation Software, version $VERSION.

## Install

The repository is not GPG-signed, so apt needs to be told to trust it:

\`\`\`bash
echo "deb [trusted=yes] https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list
sudo apt update
sudo apt install xfb
\`\`\`

## Update

\`\`\`bash
sudo apt update && sudo apt upgrade xfb
\`\`\`

## Remove

\`\`\`bash
sudo apt remove xfb
sudo rm /etc/apt/sources.list.d/xfb.list
\`\`\`

## Contents

- Architectures: $ARCHITECTURES
- Suite: stable, component: main

Packages can also be downloaded directly from the
[GitHub releases page](https://github.com/netpack/XFB/releases).

- Website: https://netpack.pt
- Issues: https://github.com/netpack/XFB/issues
EOF
touch "$REPO_DIR/.nojekyll"

echo ""
echo "✓ Repository built in $REPO_DIR/"
find "$REPO_DIR" -type f | sort | sed 's/^/    /'

echo ""
echo "=========================================="
echo "Publishing (manual)"
echo "=========================================="
echo ""
echo "GitHub Pages must be enabled for the repo first:"
echo "  Settings → Pages → Source: gh-pages branch, / (root)"
echo ""
echo "Then publish with:"
echo "  git worktree add /tmp/xfb-ghpages gh-pages"
echo "  rm -rf /tmp/xfb-ghpages/{dists,pool,README.md}"
echo "  cp -r $REPO_DIR/. /tmp/xfb-ghpages/"
echo "  git -C /tmp/xfb-ghpages add -A"
echo "  git -C /tmp/xfb-ghpages commit -m 'apt: XFB $VERSION'"
echo "  git -C /tmp/xfb-ghpages push origin gh-pages"
echo "  git worktree remove /tmp/xfb-ghpages"
echo ""
