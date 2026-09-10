#!/bin/bash
# Build the XFB .rpm using Docker, from any machine that can run it.
#
# Usage:
#   ./build-rpm-docker.sh              # builds x86_64 and aarch64
#   ./build-rpm-docker.sh x86_64       # one architecture
#
# Produces: output/xfb-<version>-1.<dist>.<arch>.rpm
#
# BUILD EACH ARCHITECTURE ON ITS OWN. The native one is the one to trust; on
# an Apple Silicon Mac that is aarch64, and x86_64 goes through qemu.
#
# Emulated x86_64 used to stop dead in rpmbuild's %prep with "Cannot open:
# Function not implemented" on the first file out of the tarball. That is GNU
# tar: under qemu it cannot extract at all, every openat() and mkdirat() it
# makes returns ENOSYS. Only extraction — writing a tarball works, cp -a
# works, an ordinary shell creating thousands of files works. bsdtar extracts
# the same tarball without an error, so Dockerfile.fedora-build now points
# rpm's %__rpmuncompress at a bsdtar shim and %prep gets through.
#
# With that shim in place an emulated x86_64 build runs all the way through:
# xfb-4.0-1.fc44.x86_64.rpm was built this way on an M-series Mac on
# 2026-09-10. It takes hours, so a real x86_64 Linux machine — the UTM Fedora
# VM with ./build-rpm.sh, or a CI runner — is still the quicker route when one
# is to hand.
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
echo "Building XFB $VERSION for: ${ARCHES[*]}"

for arch in "${ARCHES[@]}"; do
    case "$arch" in
        x86_64)  platform="linux/amd64" ;;
        aarch64) platform="linux/arm64" ;;
        *) echo "Unknown architecture: $arch (use x86_64 or aarch64)" >&2; exit 1 ;;
    esac

    echo
    echo "=== $arch ($platform) ==="
    # Building for a foreign architecture goes through qemu and is slow rather
    # than broken; on Apple silicon aarch64 is the native one.
    docker build --platform "$platform" \
        -t "xfb-rpm-builder:$arch" -f Dockerfile.fedora-build .
    docker run --rm --platform "$platform" \
        -v "$(pwd)/output:/output" "xfb-rpm-builder:$arch"
done

echo
echo "Done. In output/:"
ls -1 output/*.rpm 2>/dev/null || echo "  (nothing — check the build log above)"
