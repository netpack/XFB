#!/bin/bash
# Build XFB .deb package(s) using Docker (works from macOS)
#
# Usage:
#   ./build-deb-docker.sh            # builds both amd64 and arm64
#   ./build-deb-docker.sh amd64      # builds amd64 only
#   ./build-deb-docker.sh arm64      # builds arm64 only
#   ./build-deb-docker.sh amd64 arm64
#
# Produces: output/xfb_3.141592-1_<arch>.deb

set -e

# Architectures to build. Default to both when no args are given.
ARCHES=("$@")
if [ ${#ARCHES[@]} -eq 0 ]; then
    ARCHES=(amd64 arm64)
fi

mkdir -p output

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
for ARCH in "${ARCHES[@]}"; do
    echo "  output/xfb_3.141592-1_${ARCH}.deb"
done
echo ""
echo "To install on Debian/Ubuntu (matching your machine's architecture):"
echo "  sudo dpkg -i xfb_3.141592-1_<arch>.deb"
echo "  sudo apt-get install -f  # fix any missing dependencies"
