#!/bin/bash
# Builds the XFB flatpak bundle. Runs inside the privileged container that
# build-flatpak-docker.sh starts; not meant to be run on a desktop, where
# `flatpak-builder packaging/flatpak/pt.netpack.XFB.yml` is all you need.
#
# Produces /output/XFB-<version>-<arch>.flatpak — a single-file bundle, which
# is what a GitHub release can carry. The operator installs it with
#   flatpak install --user ./XFB-<version>-<arch>.flatpak
# and it pulls org.kde.Platform from Flathub if that runtime is not already
# there, so the machine still needs the flathub remote configured.
set -euo pipefail

ARCH="$(uname -m)"
VERSION="$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' /src/CMakeLists.txt)"
[ -n "$VERSION" ] || { echo "no version in CMakeLists.txt" >&2; exit 1; }

MANIFEST=/src/packaging/flatpak/pt.netpack.XFB.yml
RUNTIME_VERSION="$(sed -n "s/^runtime-version: *'\\([^']*\\)'.*/\\1/p" "$MANIFEST")"
[ -n "$RUNTIME_VERSION" ] || { echo "no runtime-version in the manifest" >&2; exit 1; }

echo "Building XFB $VERSION for $ARCH against org.kde.Platform//$RUNTIME_VERSION"

flatpak remote-add --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo

# -y rather than --assumeyes spelled out, and no --user: the runtime lands in
# /var/lib/flatpak, which build-flatpak-docker.sh keeps in a named volume so a
# second run does not download several gigabytes again.
flatpak install -y --noninteractive flathub \
    "org.kde.Platform//$RUNTIME_VERSION" \
    "org.kde.Sdk//$RUNTIME_VERSION"

# --disable-rofiles-fuse because rofiles-fuse wants /dev/fuse, which a
# container does not have; the flag makes flatpak-builder copy instead. It
# costs disk and time and changes nothing about the result.
flatpak-builder \
    --disable-rofiles-fuse \
    --force-clean \
    --repo=/tmp/xfb-repo \
    /tmp/xfb-build \
    "$MANIFEST"

# No branch argument: the manifest sets none, so the app is on "master" and
# that is the only branch in the repo. Naming the runtime version here instead
# would be a different thing entirely and would not resolve.
mkdir -p /output
flatpak build-bundle /tmp/xfb-repo \
    "/output/XFB-$VERSION-$ARCH.flatpak" \
    pt.netpack.XFB

echo
echo "Wrote /output/XFB-$VERSION-$ARCH.flatpak"
ls -lh "/output/XFB-$VERSION-$ARCH.flatpak"
