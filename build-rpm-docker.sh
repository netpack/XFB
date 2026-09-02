#!/bin/bash
# Build the XFB .rpm using Docker, from any machine that can run it.
#
# Usage:
#   ./build-rpm-docker.sh              # builds x86_64 and aarch64
#   ./build-rpm-docker.sh x86_64       # one architecture
#
# Produces: output/xfb-<version>-1.<dist>.<arch>.rpm
#
# BUILD EACH ARCHITECTURE ON ITS OWN. Only the native one works: an emulated
# build gets part-way and then tar starts failing on an arbitrary scattering of
# files with "Cannot open: Function not implemented", and once on rpmbuild's
# own %prep even "Cannot mkdir". Nothing in the spec can work around that — it
# is the emulation layer. On an Apple Silicon Mac that means aarch64 builds
# here and x86_64 needs a real x86_64 Linux machine (or a CI runner, which is
# x86_64 and can run this same Dockerfile natively).
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
