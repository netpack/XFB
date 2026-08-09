#!/bin/bash
# Build XFB .deb package(s) using Docker (works from macOS)
#
# Usage:
#   ./build-deb-docker.sh            # builds both amd64 and arm64
#   ./build-deb-docker.sh amd64      # builds amd64 only
#   ./build-deb-docker.sh arm64      # builds arm64 only
#   ./build-deb-docker.sh amd64 arm64
#
# Produces: output/xfb_<version>-1_<arch>.deb

set -e

# Architectures to build. Default to both when no args are given.
ARCHES=("$@")
if [ ${#ARCHES[@]} -eq 0 ]; then
    ARCHES=(amd64 arm64)
fi

mkdir -p output

# CMakeLists.txt is the single source of truth for the version (same as
# bump-version.sh reads).
VERSION=$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
if [ -z "$VERSION" ]; then
    echo "❌ Could not read the version from CMakeLists.txt"
    exit 1
fi
echo "Version from CMakeLists.txt: ${VERSION}"

# The Dockerfile does `COPY . /src`, so the packages are built from the WORKING
# TREE, not from HEAD. Uncommitted edits — including ones left behind by
# another session or a half-finished experiment — go straight into the .deb.
# Say so up front rather than let it be discovered by unpacking the package.
if command -v git >/dev/null 2>&1 && git rev-parse --git-dir >/dev/null 2>&1; then
    DIRTY=$(git status --porcelain -- src CMakeLists.txt cmake 2>/dev/null | grep -v '^??' || true)
    if [ -n "$DIRTY" ]; then
        echo ""
        echo "⚠️  The working tree has uncommitted changes that WILL be baked into"
        echo "⚠️  the package. Check this is all yours before shipping it:"
        echo "$DIRTY" | sed 's/^/     /'
    fi
fi

# Record what is already in output/ so a cached build cannot masquerade as a
# fresh one. Docker caches every layer when the build context is unchanged,
# including the layer that runs dpkg-deb — so `docker build` can succeed
# instantly and `docker run` then copies out a .deb built days ago.
declare -a PREV_SUM
for i in "${!ARCHES[@]}"; do
    DEB="output/xfb_${VERSION}-1_${ARCHES[$i]}.deb"
    if [ -f "$DEB" ]; then
        PREV_SUM[$i]=$(shasum -a 256 "$DEB" | cut -d' ' -f1)
    else
        PREV_SUM[$i]=""
    fi
done

for ARCH in "${ARCHES[@]}"; do
    case "$ARCH" in
        amd64|arm64) ;;
        *)
            echo "❌ Unsupported architecture: $ARCH (use amd64 or arm64)"
            exit 1
            ;;
    esac

    echo ""
    echo "🐧 Building XFB .deb package for ${ARCH} via Docker..."

    # Build the Docker image (compiles XFB and creates the .deb for this arch).
    # --platform drives Docker's TARGETARCH, which the Dockerfile uses for the
    # package Architecture field and the output filename.
    docker build \
        --platform "linux/${ARCH}" \
        -t "xfb-deb-builder:${ARCH}" \
        -f Dockerfile.debian-build .

    # Extract the .deb from the container into ./output
    docker run \
        --platform "linux/${ARCH}" \
        --rm \
        -v "$(pwd)/output:/output" \
        "xfb-deb-builder:${ARCH}"
done

echo ""
echo "✅ Done! Packages are in ./output:"
STALE=0
for i in "${!ARCHES[@]}"; do
    ARCH="${ARCHES[$i]}"
    DEB="output/xfb_${VERSION}-1_${ARCH}.deb"

    if [ ! -f "$DEB" ]; then
        echo "  ❌ ${DEB} — MISSING (the build did not produce it)"
        STALE=1
        continue
    fi

    SUM=$(shasum -a 256 "$DEB" | cut -d' ' -f1)
    SIZE=$(wc -c < "$DEB" | tr -d ' ')

    # A genuine Docker build is never bit-for-bit reproducible (timestamps and
    # paths are baked in), so an unchanged checksum means the image — and the
    # .deb inside it — came from the layer cache and nothing was recompiled.
    if [ -n "${PREV_SUM[$i]}" ] && [ "${PREV_SUM[$i]}" = "$SUM" ]; then
        echo "  ⚠️  ${DEB}"
        echo "      ${SIZE} bytes, sha256 ${SUM:0:16}…"
        echo "      UNCHANGED from before this run — served from the Docker layer"
        echo "      cache, NOT rebuilt. That is correct only if nothing in the"
        echo "      build context changed. If you expected new code in here, the"
        echo "      edit never reached the context (wrong tree? .dockerignore?)."
        STALE=1
    else
        echo "  ✅ ${DEB}"
        echo "      ${SIZE} bytes, sha256 ${SUM:0:16}…"
    fi

    # The package must carry the version we think we are shipping.
    DEB_VER=$(dpkg-deb -f "$DEB" Version 2>/dev/null || true)
    if [ -z "$DEB_VER" ] && command -v ar >/dev/null 2>&1; then
        DEB_VER=$(TMP=$(mktemp -d) && cd "$TMP" && ar x "$OLDPWD/$DEB" 2>/dev/null \
                  && tar xf control.tar.xz 2>/dev/null \
                  && sed -n 's/^Version: //p' control; rm -rf "$TMP")
    fi
    # The 1: epoch comes from Dockerfile.debian-build and is part of the real
    # package version, so it has to be part of what we check against.
    if [ -n "$DEB_VER" ] && [ "$DEB_VER" != "1:${VERSION}-1" ]; then
        echo "      ❌ package says Version: ${DEB_VER}, expected 1:${VERSION}-1"
        STALE=1
    fi
done

if [ "$STALE" -ne 0 ]; then
    echo ""
    echo "⚠️  At least one package was not freshly built or does not match the"
    echo "⚠️  expected version. Do NOT publish these without checking."
    echo "⚠️  To force a genuine rebuild:"
    echo "⚠️    docker image rm xfb-deb-builder:amd64 xfb-deb-builder:arm64"
    echo "⚠️    ./build-deb-docker.sh"
fi

echo ""
echo "To install on Debian/Ubuntu (matching your machine's architecture):"
echo "  sudo dpkg -i xfb_${VERSION}-1_<arch>.deb"
echo "  sudo apt-get install -f  # fix any missing dependencies"
