#!/usr/bin/env bash
#
# Builds the XFB .rpm natively, on a Fedora machine.
#
# This is the one to use inside a Fedora VM or on a real Fedora box.
# build-rpm-docker.sh is for building from a machine that is not Fedora;
# there is no reason to run Docker inside Fedora to build for Fedora.
#
#   ./build-rpm.sh
#
# Produces: output/xfb-<version>-1.<dist>.<arch>.rpm
#
# It needs to install build dependencies the first time, which wants root.
# Everything else runs as you.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

# --- is this even Fedora? ----------------------------------------------------
if ! command -v rpmbuild >/dev/null 2>&1 && ! command -v dnf >/dev/null 2>&1; then
    echo "This does not look like an RPM distribution: no dnf and no rpmbuild." >&2
    echo "On macOS or Debian, use ./build-rpm-docker.sh instead." >&2
    exit 1
fi

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    command -v sudo >/dev/null 2>&1 || { echo "Need root (or sudo) to install build dependencies." >&2; exit 1; }
    SUDO="sudo"
fi

# --- version -----------------------------------------------------------------
# CMakeLists.txt is the single source of truth, the same place bump-version.sh
# and the Debian and Docker builds read it from.
VERSION=$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$VERSION" ] || { echo "Could not read the version out of CMakeLists.txt." >&2; exit 1; }
echo "Building XFB $VERSION for $(rpm --eval '%{_arch}' 2>/dev/null || uname -m)"

# --- build dependencies ------------------------------------------------------
# Read out of the spec by dnf rather than repeated here, so the two cannot
# drift. The first run pulls a few hundred packages and is the slow part.
echo
echo "== Installing build dependencies (first run only) =="
$SUDO dnf -y install rpm-build rpmdevtools 'dnf-command(builddep)' >/dev/null 2>&1 || \
    $SUDO dnf -y install rpm-build rpmdevtools dnf-plugins-core
# --define is needed here too: the spec's Version is %{xfb_version}, and a
# spec that will not parse has no BuildRequires to read.
$SUDO dnf -y builddep --define "xfb_version $VERSION" packaging/xfb.spec

# --- stage the source --------------------------------------------------------
# rpmbuild wants a tarball whose top directory is name-version, so the tree is
# copied under that name rather than built in place. Copied rather than piped
# through tar: the same staging is used by the Docker build, where a tar of a
# tree this size fails under emulation.
rpmdevtools_root="$(rpm --eval '%{_topdir}')"
rpmdev-setuptree
staging="$(mktemp -d)"
trap 'rm -rf "$staging"' EXIT

echo
echo "== Staging the source =="
# Excluded rather than copied and deleted: a working tree that has built
# anything holds Windows installers, dist trees and the packaging checkouts,
# which together are several GB and none of which the build reads. The list
# mirrors .dockerignore, which exists for the same reason.
mkdir -p "$staging/xfb-$VERSION"
tar -cf - \
    --exclude='./.git' --exclude='./.claude' --exclude='./output' \
    --exclude='./pkg-root' --exclude='./build' --exclude='./build-*' \
    --exclude='./dist' --exclude='./dist-*' --exclude='./XFB.app' \
    --exclude='./aur-xfb' --exclude='./homebrew-xfb' --exclude='./debian-repo' \
    --exclude='./tmp' --exclude='./tmp_verify' \
    --exclude='*.exe' --exclude='*.dmg' --exclude='*.7z' --exclude='*.deb' \
    --exclude='*.rpm' --exclude='*.pkg.tar.zst' \
    -C "$here" . | tar -xf - -C "$staging/xfb-$VERSION"

tar -czf "$rpmdevtools_root/SOURCES/xfb-$VERSION.tar.gz" -C "$staging" "xfb-$VERSION"

# --- build -------------------------------------------------------------------
echo
echo "== Building (this is the long part) =="
rpmbuild -bb --define "xfb_version $VERSION" packaging/xfb.spec

# --- collect -----------------------------------------------------------------
mkdir -p output
find "$rpmdevtools_root/RPMS" -name "xfb-$VERSION-*.rpm" -newermt '-1 hour' -exec cp -v {} output/ \;

echo
echo "Done. In output/:"
ls -1 output/*.rpm 2>/dev/null || echo "  (nothing — check the build log above)"
echo
built="$(ls -1 output/xfb-"$VERSION"-*.rpm 2>/dev/null | grep -v debug | head -1 || true)"
if [ -n "$built" ]; then
    echo "Install it with:"
    echo "  sudo dnf install ./$built"
fi
echo
if [ ! -f packaging/companion/xfb-companion.apk ]; then
    echo "Note: packaging/companion/xfb-companion.apk is not here (it is gitignored,"
    echo "being a build product of the Android toolchain), so this package will not"
    echo "carry the phone app. The desktop works fine without it and simply says it"
    echo "has no app to hand out."
fi
