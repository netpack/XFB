#!/bin/bash
# Bump the XFB version everywhere, in one shot, BEFORE running any build script.
#
#   ./bump-version.sh 3.1415926535           bump to this version
#   ./bump-version.sh --dry-run 3.1415926535 show what would change
#   ./bump-version.sh --show                 print the current version
#   ./bump-version.sh --touch-date           set CITATION.cff date-released to today
#
# Why this exists: the version lives in 16 places across CMake, three build
# scripts, the NSIS installer, PKGBUILD, the Dockerfile, the publish scripts,
# the Homebrew cask template and the README. Bumping them by hand means one gets
# missed, and the way that usually surfaces is a full rebuild that still carries
# the PREVIOUS version — artifacts that cannot be published, because that number
# is already live on GitHub/AUR/Homebrew/winget with pinned sha256 sums.
#
# Beyond substituting, it:
#   * refuses to bump to a version that is already tagged or released, which is
#     the exact mistake this is meant to stop;
#   * fails loudly if any expected occurrence is missing, so a silent partial
#     bump cannot happen — add new spots to the table, never by hand;
#   * regenerates the committed src/ui_*.h headers, which shadow AUTOUIC.
#
# It deliberately does NOT touch:
#   * the pi literals in player.cpp / FxDsp.cpp (3.14159265358979323846) — every
#     match below is a literal that includes its surrounding syntax, so a bare
#     digit run can never be hit;
#   * sha256 sums — the artifacts do not exist yet. update-homebrew-tap.sh
#     computes the dmg hash itself and PKGBUILD uses SKIP.

set -euo pipefail
cd "$(dirname "$0")"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
ok()   { echo -e "${GREEN}✓${NC} $1"; }
warn() { echo -e "${YELLOW}⚠${NC} $1"; }
err()  { echo -e "${RED}✗${NC} $1"; }
info() { echo -e "${BLUE}·${NC} $1"; }

# CMakeLists.txt is the single source of truth for the current version.
CURRENT="$(sed -n 's/^project(XFB VERSION \([0-9.]*\) LANGUAGES CXX)$/\1/p' CMakeLists.txt | head -1)"
if [ -z "$CURRENT" ]; then
    err "Could not read the current version from CMakeLists.txt."
    err "Expected: project(XFB VERSION <x.y> LANGUAGES CXX)"
    exit 1
fi

DRY_RUN=0
NEW=""
for arg in "$@"; do
    case "$arg" in
        --dry-run|-n)  DRY_RUN=1 ;;
        --show|-s)     echo "$CURRENT"; exit 0 ;;
        --touch-date)  TODAY="$(date +%F)"
                       sed -i.bak "s/^date-released: .*/date-released: $TODAY/" CITATION.cff
                       rm -f CITATION.cff.bak
                       ok "CITATION.cff date-released set to $TODAY"
                       exit 0 ;;
        -h|--help)     sed -n '2,27p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        -*)            err "Unknown option: $arg"; exit 1 ;;
        *)             NEW="$arg" ;;
    esac
done

if [ -z "$NEW" ]; then
    err "Usage: ./bump-version.sh [--dry-run] <new-version>"
    info "current version is $CURRENT"
    exit 1
fi

if ! [[ "$NEW" =~ ^[0-9]+(\.[0-9]+)+$ ]]; then
    err "'$NEW' is not a dotted numeric version (e.g. 3.1415926535)."
    exit 1
fi
if [ "$NEW" = "$CURRENT" ]; then
    err "New version is identical to the current one ($CURRENT)."
    err "This is precisely the case the script exists to catch: rebuilding under"
    err "an already-published version produces artifacts you cannot ship."
    exit 1
fi

# XFB versions grow by appending digits of pi, so compare as digit strings
# padded to equal length rather than as numbers or plain strings.
if awk -v a="$NEW" -v b="$CURRENT" 'BEGIN{
        gsub(/\./,"",a); gsub(/\./,"",b);
        while (length(a)<length(b)) a=a "0";
        while (length(b)<length(a)) b=b "0";
        exit !(a<b) }'; then
    warn "$NEW is LOWER than the current $CURRENT."
    read -r -p "  Bump downwards anyway? (y/N) " reply
    [[ "$reply" =~ ^[Yy]$ ]] || { info "Aborted."; exit 1; }
fi

# --- Refuse to reuse an already-published version ----------------------------
# Once v<x> is tagged or released its artifacts are pinned by sha256 in the
# Homebrew cask and the winget manifest; re-uploading under the same number
# breaks every one of those installs.
if git rev-parse -q --verify "refs/tags/v$NEW" >/dev/null 2>&1; then
    err "Tag v$NEW already exists locally — that version has been released."
    exit 1
fi
if git ls-remote --exit-code --tags origin "refs/tags/v$NEW" >/dev/null 2>&1; then
    err "Tag v$NEW already exists on origin — that version is public."
    exit 1
fi
if command -v gh >/dev/null 2>&1; then
    if gh release view "v$NEW" --repo netpack/XFB >/dev/null 2>&1; then
        err "GitHub release v$NEW already exists — its assets are already published."
        exit 1
    fi
    ok "v$NEW is not tagged and has no GitHub release"
else
    warn "gh not found — could not check whether v$NEW is already released."
fi

echo ""
echo "═══ XFB version bump:  $CURRENT  →  $NEW ═══"
[ "$DRY_RUN" = 1 ] && warn "DRY RUN — no files will be written"
echo ""

# --- The substitution table --------------------------------------------------
# One line per spot:   <file> | <expected count> | <literal snippet with @V@>
# @V@ is replaced by the old version to find, and by the new one to write.
# @N@ is the same version in the four-integer form a Windows version resource
# needs — <major>.<how many decimals>.0.0, matching what CMakeLists.txt derives
# for XFB.exe. Each snippet carries enough surrounding syntax that it can only
# match the real version. To add a spot, add a line — do not edit files by hand.
TABLE='
CMakeLists.txt            | 1 | project(XFB VERSION @V@ LANGUAGES CXX)
src/main.cpp              | 1 | XFB_VERSION = "@V@"
build-macos.sh            | 1 | VERSION="@V@"
build-windows.bat         | 1 | set VERSION=@V@
release.sh                | 1 | VERSION="@V@"
installer.nsi             | 1 | !define VERSION "@V@"
installer.nsi             | 1 | !define VERSIONNUM "@N@"
PKGBUILD                  | 1 | pkgver=@V@
PKGBUILD                  | 1 | New in v@V@:
update-aur.sh             | 1 | VERSION="@V@"
update-aur.sh             | 1 | Updated to XFB @V@
update-homebrew-tap.sh    | 1 | VERSION="${1:-@V@}"
update-debian-repo.sh     | 1 | VERSION="${1:-@V@}"
Dockerfile.debian-build   | 1 | Version: @V@-1
Dockerfile.debian-build   | 3 | xfb_@V@-1_
packaging/homebrew/xfb.rb | 1 | version "@V@"
README.md                 | 5 | @V@
CITATION.cff              | 1 | version: @V@
'

# CITATION.cff also carries a release date, which no @V@ substitution can
# produce. It is set to today alongside the bump; if the release slips, re-run
# ./bump-version.sh --touch-date before tagging.
# .zenodo.json deliberately has no version field — Zenodo takes it from the
# GitHub release tag, so there is nothing to bump there.

# "3.14159265358" -> "3.1415.9265.3580", "3.1416" -> "3.1416.0.0". Windows
# compares versions as four 16-bit integers, so the digits cannot go in
# directly. Reading them the way the decimal reads — the first twelve digits
# after the point in three four-digit groups — keeps Windows' ordering the same
# as the version's own. A digit COUNT would not: 3.1416 is newer than
# 3.14159265358 but has seven fewer decimals. Must match CMakeLists.txt.
numeric_version() {
    printf '%s' "$1" | awk -F. '{
        frac = substr($2 "000000000000", 1, 12)
        printf "%s.%d.%d.%d", $1, substr(frac,1,4)+0, substr(frac,5,4)+0, substr(frac,9,4)+0
    }'
}
CURRENT_NUM="$(numeric_version "$CURRENT")"
NEW_NUM="$(numeric_version "$NEW")"

export BUMP_CURRENT="$CURRENT" BUMP_NEW="$NEW" BUMP_DRY="$DRY_RUN" BUMP_TABLE="$TABLE"
export BUMP_CURRENT_NUM="$CURRENT_NUM" BUMP_NEW_NUM="$NEW_NUM"

python3 - <<'PYEOF'
import os, sys, re, datetime

cur, new = os.environ['BUMP_CURRENT'], os.environ['BUMP_NEW']
curnum, newnum = os.environ['BUMP_CURRENT_NUM'], os.environ['BUMP_NEW_NUM']
dry = os.environ['BUMP_DRY'] == '1'

def fill(snippet, version, numeric):
    return snippet.replace('@V@', version).replace('@N@', numeric)

G, R, Y, N = '\033[0;32m', '\033[0;31m', '\033[1;33m', '\033[0m'

# Group by file so each file is read once and written once.
plan = {}
order = []
for line in os.environ['BUMP_TABLE'].strip().splitlines():
    path, want, snippet = (p.strip() for p in line.split('|', 2))
    if path not in plan:
        plan[path] = []
        order.append(path)
    plan[path].append((int(want), snippet))

failed = False
for path in order:
    if not os.path.isfile(path):
        print(f"{R}✗{N} {path} — missing")
        failed = True
        continue

    with open(path, encoding='utf-8') as fh:
        text = original = fh.read()

    total = 0
    for want, snippet in plan[path]:
        old = fill(snippet, cur, curnum)
        found = text.count(old)
        if found == 0:
            print(f"{R}✗{N} {path} — not found: {old!r}")
            failed = True
            continue
        if found != want:
            print(f"{Y}⚠{N} {path} — {old!r}: found {found}, expected {want}")
        text = text.replace(old, fill(snippet, new, newnum))
        total += found

    if text == original:
        continue
    if not dry:
        with open(path, 'w', encoding='utf-8') as fh:
            fh.write(text)
    print(f"{G}✓{N} {path} — {total} occurrence(s)")

# --- CITATION.cff date-released ----------------------------------------------
# Not a version substitution, so it cannot live in the table. Set to the bump
# date; ./bump-version.sh --touch-date resets it if the release slips.
today = datetime.date.today().isoformat()
if os.path.isfile('CITATION.cff'):
    with open('CITATION.cff', encoding='utf-8') as fh:
        text = original = fh.read()
    text, n = re.subn(r'(?m)^date-released: .*$', f'date-released: {today}', text)
    if n == 0:
        print(f"{R}✗{N} CITATION.cff — no date-released line")
        failed = True
    elif text != original:
        if not dry:
            with open('CITATION.cff', 'w', encoding='utf-8') as fh:
                fh.write(text)
        print(f"{G}✓{N} CITATION.cff — date-released → {today}")

sys.exit(1 if failed else 0)
PYEOF

echo ""

if [ "$DRY_RUN" = 1 ]; then
    info "Dry run finished — nothing written."
    info "(The leftover scan only runs on a real bump, where it is meaningful.)"
    exit 0
fi

# --- Anything left behind? ---------------------------------------------------
# Catches a spot that exists in the project but not in the table. Requiring a
# non-digit after the version is what keeps the pi literals out of the results.
# Excluded directories are all DERIVED and are refreshed by their own scripts:
#   aur-xfb/      — update-aur.sh copies PKGBUILD in and regenerates .SRCINFO
#   debian-repo/  — update-debian-repo.sh regenerates the Packages indexes
#   homebrew-xfb/ — update-homebrew-tap.sh rewrites the cask from the template
#   build*/       — CMake build trees (CPack config, CMakeCache)
echo "── Scanning for leftover occurrences of $CURRENT ──"
LEFTOVER="$(grep -rn --binary-files=without-match "${CURRENT//./\\.}\([^0-9]\|$\)" \
    --exclude-dir=.git --exclude-dir=.claude \
    --exclude-dir=build --exclude-dir=build-cmake \
    --exclude-dir=build-windows-x64 --exclude-dir=build-windows-arm64 \
    --exclude-dir=output --exclude-dir=dist \
    --exclude-dir=dist-windows-x64 --exclude-dir=dist-windows-arm64 \
    --exclude-dir=aur-xfb --exclude-dir=debian-repo --exclude-dir=homebrew-xfb \
    --exclude='*.ts' --exclude='*.qm' --exclude='*.o' --exclude='*.log' \
    . 2>/dev/null || true)"

if [ -n "$LEFTOVER" ]; then
    warn "Still mentioning $CURRENT — review, and add to the table if it should bump:"
    printf '%s\n' "$LEFTOVER" | sed 's/^/    /'
else
    ok "No leftover references to $CURRENT"
fi

# --- Regenerate the committed ui_*.h headers ---------------------------------
# They live in src/ and shadow AUTOUIC, so a stale one silently wins over its .ui.
UIC=""
for cand in /opt/homebrew/opt/qt/share/qt/libexec/uic "$HOME"/Qt/*/macos/libexec/uic; do
    [ -x "$cand" ] && { UIC="$cand"; break; }
done
if [ -n "$UIC" ]; then
    for ui in src/*.ui; do
        header="src/ui_$(basename "${ui%.ui}").h"
        [ -f "$header" ] && "$UIC" "$ui" -o "$header"
    done
    ok "src/ui_*.h headers regenerated"
else
    warn "uic not found — if you edited any .ui, regenerate src/ui_*.h yourself."
fi

echo ""
ok "Version bumped to $NEW"
echo ""
echo "Next:"
echo "  1. ./build-macos.sh                 → dmg"
echo "  2. ./build-deb-docker.sh            → both debs (a macOS build does NOT prove the deb builds)"
echo "  3. build-windows.bat on Windows     → x64 + arm64 installers"
echo "  4. check every artifact self-reports $NEW, then publish"
echo ""
warn "Cask/winget hashes are filled in after the artifacts exist —"
warn "update-homebrew-tap.sh $NEW computes the dmg sha256 itself."
