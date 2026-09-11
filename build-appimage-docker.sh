#!/bin/bash
# Build the XFB AppImage using Docker, from any machine that can run it.
#
# Usage:
#   ./build-appimage-docker.sh             # builds x86_64 and aarch64
#   ./build-appimage-docker.sh x86_64      # one architecture
#
# Produces: output/XFB-<version>-<arch>.AppImage
#
# One file the operator downloads, marks executable and runs — no package
# manager, no root, nothing installed. It carries Qt, ffmpeg and ffprobe, so
# the only thing it needs from the host is glibc 2.36 or newer (Debian 12,
# Ubuntu 24.04, Fedora 37 and up) and, to mount itself, FUSE 2 — libfuse2 on
# Debian/Ubuntu, fuse-libs on Fedora. An AppImage on a host without it fails
# with "dlopen(): error loading libfuse.so.2"; `./XFB-<version>-<arch>.AppImage
# --appimage-extract-and-run` works anyway.
#
# Building for a foreign architecture goes through qemu and is slow rather than
# broken, with one exception worth knowing: GNU tar cannot extract under qemu
# at all, so Dockerfile.appimage unpacks the ffmpeg archive with bsdtar. See
# build-rpm-docker.sh for the same fault in rpmbuild's %prep.
set -e

ARCHES=("$@")
if [ ${#ARCHES[@]} -eq 0 ]; then
    ARCHES=(x86_64 aarch64)
fi

mkdir -p output

VERSION=$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
if [ -z "$VERSION" ]; then
    echo "Could not read the version out of CMakeLists.txt." >&2
    exit 1
fi
echo "Building the XFB $VERSION AppImage for: ${ARCHES[*]}"

for arch in "${ARCHES[@]}"; do
    case "$arch" in
        x86_64)  platform="linux/amd64" ;;
        aarch64) platform="linux/arm64" ;;
        *) echo "Unknown architecture: $arch (use x86_64 or aarch64)" >&2; exit 1 ;;
    esac

    echo
    echo "=== $arch ($platform) ==="
    docker build --platform "$platform" \
        -t "xfb-appimage-builder:$arch" -f Dockerfile.appimage .
    docker run --rm --platform "$platform" \
        -v "$(pwd)/output:/output" "xfb-appimage-builder:$arch"
done

echo
echo "Done. In output/:"
ls -1 output/*.AppImage 2>/dev/null || echo "  (nothing — check the build log above)"
