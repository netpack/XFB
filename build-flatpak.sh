#!/usr/bin/env bash
#
# Builds the XFB flatpak natively, on a Linux machine.
#
# This is the one to use inside the Fedora VM or on any real Linux box.
# build-flatpak-docker.sh is for building from a machine that is not Linux;
# there is no reason to run Docker inside Linux to build for Linux, and there
# is one strong reason not to on an Apple Silicon Mac: see below.
#
#   ./build-flatpak.sh
#
# Produces: output/XFB-<version>-<arch>.flatpak — a single-file bundle. Install
# it with `flatpak install --user ./XFB-<version>-<arch>.flatpak`; the machine
# needs the flathub remote configured, because the bundle carries XFB but not
# org.kde.Platform.
#
# WHY THE X86_64 BUNDLE HAS TO BE BUILT SOMEWHERE LIKE THIS. flatpak-builder
# sandboxes every module with bubblewrap, and bubblewrap always installs a
# seccomp filter. A seccomp filter is a BPF program validated against the
# native syscall ABI, so an x86_64 process running on an arm64 kernel cannot
# install one — prctl(PR_SET_SECCOMP) returns EINVAL — however good the
# instruction translation underneath is. It is not a gap in the emulator that a
# better one would close, and flatpak-builder has no flag to skip the filter.
# Run it on a machine whose architecture is the one you are building for.
#
# NOTHING HEAVY IS WRITTEN INTO THE SOURCE TREE, and that is deliberate rather
# than tidiness. The VM that builds x86_64 reaches this checkout over a shared
# folder, and ostree cannot create a repository on one — it wants hardlinks and
# xattrs a virtiofs/9p share does not provide. flatpak-builder's state
# directory holds an ostree repo, so left at its default (.flatpak-builder in
# the current directory) the build dies with
#
#   Error opening cache: opening repo: opendir(objects): No such file or directory
#
# after downloading every source perfectly well, which reads like a corrupt
# cache and is not one. The state directory, the build tree and the output repo
# therefore all live under $XDG_CACHE_HOME, and so does the bundle while it is
# being written -- `flatpak build-bundle` wants open(O_TMPFILE), which 9p does
# not implement either, so only a plain `cp` of the finished file touches
# output/. Override with XFB_FLATPAK_CACHE if that path is itself on a share.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

if ! command -v flatpak-builder >/dev/null 2>&1; then
    echo "flatpak-builder is not installed." >&2
    echo "  Fedora:        sudo dnf install flatpak flatpak-builder" >&2
    echo "  Debian/Ubuntu: sudo apt install flatpak flatpak-builder" >&2
    echo "On macOS, use ./build-flatpak-docker.sh instead." >&2
    exit 1
fi

ARCH="$(uname -m)"

VERSION=$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$VERSION" ] || { echo "Could not read the version out of CMakeLists.txt." >&2; exit 1; }

MANIFEST="$here/packaging/flatpak/pt.netpack.XFB.yml"
RUNTIME_VERSION="$(sed -n "s/^runtime-version: *'\\([^']*\\)'.*/\\1/p" "$MANIFEST")"
[ -n "$RUNTIME_VERSION" ] || { echo "No runtime-version in the manifest." >&2; exit 1; }

echo "Building XFB $VERSION for $ARCH against org.kde.Platform//$RUNTIME_VERSION"

# --user throughout, so none of this needs root.
flatpak remote-add --user --if-not-exists flathub \
    https://flathub.org/repo/flathub.flatpakrepo

echo
echo "== Runtime and SDK (first run only, about 3 GB) =="
flatpak install --user -y --noninteractive flathub \
    "org.kde.Platform//$RUNTIME_VERSION" \
    "org.kde.Sdk//$RUNTIME_VERSION"

echo
echo "== Building =="
CACHE_ROOT="${XFB_FLATPAK_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/xfb-flatpak}"
mkdir -p "$CACHE_ROOT"
STATE_DIR="$CACHE_ROOT/state"
BUILD_DIR="$CACHE_ROOT/build"
REPO_DIR="$CACHE_ROOT/repo"
echo "   (working in $CACHE_ROOT, off the source tree — see the note at the top)"

# A state directory left behind by a failed run keeps failing the same way:
# flatpak-builder finds cache/ already there, empty, and will not initialise
# over it. Cheap to spot and cheap to fix.
if [ -d "$STATE_DIR/cache" ] && [ ! -d "$STATE_DIR/cache/objects" ]; then
    echo "   (clearing a half-made ostree cache from an earlier run)"
    rm -rf "$STATE_DIR/cache"
fi

flatpak-builder --user --force-clean \
    --state-dir="$STATE_DIR" \
    --repo="$REPO_DIR" \
    "$BUILD_DIR" "$MANIFEST"

echo
echo "== Bundling =="
# The bundle is written under $CACHE_ROOT and copied out, for the same reason
# everything else here is -- and the note above was wrong to call the finished
# bundle "an ordinary file". `flatpak build-bundle` creates its output with
# open(O_TMPFILE), an unnamed temporary in the destination directory that is
# given a name once it is complete, and 9p does not implement it:
#
#   == Bundling ==
#   error: open(O_TMPFILE): No such file or directory
#
# after the whole build has succeeded and both commits are already in the repo,
# which is a galling place to stop. `cp` uses an ordinary create and is fine, so
# only build-bundle has to be kept off the share.
mkdir -p "$here/output"
BUNDLE="$here/output/XFB-$VERSION-$ARCH.flatpak"
STAGED_BUNDLE="$CACHE_ROOT/XFB-$VERSION-$ARCH.flatpak"
rm -f "$STAGED_BUNDLE"
flatpak build-bundle "$REPO_DIR" "$STAGED_BUNDLE" pt.netpack.XFB
cp "$STAGED_BUNDLE" "$BUNDLE"
rm -f "$STAGED_BUNDLE"

echo
echo "Done:"
ls -lh "$BUNDLE"
echo
echo "Try it with:"
echo "  flatpak install --user $BUNDLE"
echo "  flatpak run pt.netpack.XFB"
