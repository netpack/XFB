#!/bin/bash
# Build the static flatpak repository for GitHub Pages.
#
# Turns the .flatpak bundles in output/ into an ostree repository that an
# operator adds once and then updates like any other remote:
#
#   flatpak install https://netpack.github.io/XFB/flatpak/xfb.flatpakref
#   flatpak update
#
# That is the whole reason this exists. A bundle installed by path has no
# origin, so `flatpak update` never offers the next release to it; installed
# from this repo, the origin is "xfb" and every release after arrives on its
# own. The same trade already made for Debian with update-debian-repo.sh.
#
# The ostree tooling does not exist on macOS, so the repository is built inside
# a Fedora container. It is built on the container's own filesystem and only
# copied out at the end: ostree cannot create a repository on a shared folder
# (it wants hardlinks and xattrs), and a Docker bind mount is one.
#
# Rebuilt from nothing every release rather than accumulated. gh-pages is a
# plain git branch with a soft 1 GB limit, and at ~40 MB a release an
# accumulating repo would reach it in a couple of dozen releases. Clients do
# not need the old commits to update; they compare what the ref points at.
#
# Usage:
#   ./update-flatpak-repo.sh [version]
#
# Publishing to gh-pages is deliberately NOT automatic; see the notes printed
# at the end.

set -e

VERSION="${1:-$(sed -n 's/^project(XFB VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)}"
REPO_DIR="flatpak-repo"
PAGES_URL="https://netpack.github.io/XFB/flatpak/"

echo "=========================================="
echo "Building flatpak repository for XFB $VERSION"
echo "=========================================="
echo ""

if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Start Docker Desktop and try again."
    exit 1
fi

# --- Locate the bundles -----------------------------------------------------
BUNDLES=()
for arch in x86_64 aarch64; do
    bundle="output/XFB-${VERSION}-${arch}.flatpak"
    if [ -f "$bundle" ]; then
        BUNDLES+=("$bundle")
        echo "✓ $bundle"
    else
        echo "⚠  $bundle not found — that architecture will be missing from the repo"
    fi
done

if [ ${#BUNDLES[@]} -eq 0 ]; then
    echo ""
    echo "❌ No bundles for $VERSION in output/."
    echo "   Build them with ./build-flatpak.sh (on Linux) or ./build-flatpak-docker.sh."
    exit 1
fi

# A repo with one architecture missing is not an error, but it is a quiet way
# to strand every operator on the other one, so it has to be asked for.
if [ ${#BUNDLES[@]} -lt 2 ]; then
    echo ""
    read -p "Publish a repository with only one architecture? (y/N) " -n 1 -r; echo
    [[ $REPLY =~ ^[Yy]$ ]] || { echo "Cancelled."; exit 1; }
fi

rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"

# --- Build the repository in a container ------------------------------------
echo ""
echo "Importing bundles and generating the summary (in a Fedora container)..."

docker run --rm --platform linux/arm64 \
    -v "$PWD/output:/in:ro" \
    -v "$PWD/$REPO_DIR:/out" \
    -e VERSION="$VERSION" \
    fedora:44 bash -c '
set -e
dnf -y -q install flatpak ostree >/dev/null 2>&1

repo=/tmp/repo
# archive-z2 is the mode meant for serving over plain HTTP: every object is a
# compressed file of its own, so a static host is all it needs.
ostree init --repo="$repo" --mode=archive-z2

for bundle in /in/XFB-"$VERSION"-*.flatpak; do
    [ -f "$bundle" ] || continue
    flatpak build-import-bundle "$repo" "$bundle"
done

# The summary is what a client reads to learn which refs exist; the appstream
# branches give `flatpak search` and `flatpak remote-ls` a name to show rather
# than a bare app id. Without this step the repository is unusable.
flatpak build-update-repo --prune "$repo"

cp -r "$repo"/. /out/
chown -R "$(stat -c %u:%g /out)" /out
'

# --- The two files an operator actually touches ----------------------------
# Neither carries a GPGKey. The repository is unsigned, and flatpak then adds
# the remote with gpg-verify off by itself — no flag for the operator to type.
# The Debian repo is unsigned for the same reason. RuntimeRepo is what lets a
# machine without Flathub configured still find org.kde.Platform.
cat > "$REPO_DIR/xfb.flatpakrepo" <<EOF
[Flatpak Repo]
Title=XFB
Url=${PAGES_URL}
Homepage=https://github.com/netpack/XFB
Comment=Radio automation, playout and music library management
Description=XFB is a radio automation and playout system, built to be used without sight.
RuntimeRepo=https://dl.flathub.org/repo/flathub.flatpakrepo
EOF

cat > "$REPO_DIR/xfb.flatpakref" <<EOF
[Flatpak Ref]
Title=XFB
Name=pt.netpack.XFB
Branch=master
Url=${PAGES_URL}
Homepage=https://github.com/netpack/XFB
Comment=Radio automation, playout and music library management
IsRuntime=false
RuntimeRepo=https://dl.flathub.org/repo/flathub.flatpakrepo
SuggestRemoteName=xfb
EOF

cat > "$REPO_DIR/index.html" <<EOF
<!doctype html>
<meta charset="utf-8">
<title>XFB flatpak repository</title>
<h1>XFB flatpak repository</h1>
<p>Install XFB, and receive every later release through <code>flatpak update</code>:</p>
<pre>flatpak install ${PAGES_URL}xfb.flatpakref</pre>
<p>Or add the repository and install from it by name:</p>
<pre>flatpak remote-add --user --if-not-exists xfb ${PAGES_URL}xfb.flatpakrepo
flatpak install --user xfb pt.netpack.XFB</pre>
<p>The repository is not signed. Architectures: x86_64 and aarch64.
<a href="https://github.com/netpack/XFB">github.com/netpack/XFB</a></p>
EOF

echo ""
echo "✓ Repository built in $REPO_DIR/"
echo "  refs:"
find "$REPO_DIR/refs/heads/app" -type f 2>/dev/null | sed "s|$REPO_DIR/refs/heads/|    |"
echo "  size: $(du -sh "$REPO_DIR" | cut -f1), $(find "$REPO_DIR" -type f | wc -l | tr -d ' ') files"

echo ""
echo "=========================================="
echo "Publishing (manual)"
echo "=========================================="
echo ""
echo "It lives under /flatpak on the same gh-pages branch as the apt repository,"
echo "so publishing must replace that directory and leave everything else alone:"
echo ""
echo "  git worktree add /tmp/xfb-ghpages gh-pages"
echo "  rm -rf /tmp/xfb-ghpages/flatpak"
echo "  cp -r $REPO_DIR /tmp/xfb-ghpages/flatpak"
echo "  git -C /tmp/xfb-ghpages add -A flatpak"
echo "  git -C /tmp/xfb-ghpages commit -m 'flatpak: XFB $VERSION'"
echo "  git -C /tmp/xfb-ghpages push origin gh-pages"
echo "  git worktree remove /tmp/xfb-ghpages"
echo ""
