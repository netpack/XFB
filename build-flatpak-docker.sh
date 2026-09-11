#!/bin/bash
# Build the XFB flatpak using Docker, from any machine that can run it.
#
# Usage:
#   ./build-flatpak-docker.sh              # builds x86_64 and aarch64
#   ./build-flatpak-docker.sh x86_64       # one architecture
#
# Produces: output/XFB-<version>-<arch>.flatpak — a single-file bundle, which
# is what a GitHub release can carry. Install it with:
#   flatpak install --user ./XFB-<version>-<arch>.flatpak
# The machine needs the flathub remote configured, because the bundle carries
# XFB but not org.kde.Platform.
#
# --privileged is not optional. flatpak-builder sandboxes each module it builds
# with bubblewrap, which needs to create user namespaces and mounts; without it
# the build fails on the first module with "bwrap: No permissions to creating
# new namespace". The container is throwaway and builds only this source tree.
#
# The runtime and SDK are about 3 GB and land in /var/lib/flatpak, which is
# kept in a named docker volume so a second run does not fetch them again.
# Remove it with: docker volume rm xfb-flatpak-cache
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
echo "Building the XFB $VERSION flatpak for: ${ARCHES[*]}"

for arch in "${ARCHES[@]}"; do
    case "$arch" in
        x86_64)  platform="linux/amd64" ;;
        aarch64) platform="linux/arm64" ;;
        *) echo "Unknown architecture: $arch (use x86_64 or aarch64)" >&2; exit 1 ;;
    esac

    echo
    echo "=== $arch ($platform) ==="
    docker build --platform "$platform" \
        -t "xfb-flatpak-builder:$arch" -f Dockerfile.flatpak-build .
    docker run --rm --privileged --platform "$platform" \
        -v "xfb-flatpak-cache-$arch:/var/lib/flatpak" \
        -v "$(pwd)/output:/output" "xfb-flatpak-builder:$arch"
done

echo
echo "Done. In output/:"
ls -1 output/*.flatpak 2>/dev/null || echo "  (nothing — check the build log above)"
