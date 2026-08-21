#!/usr/bin/env bash
#
# Builds the companion's release APK and puts it where XFB hands it out.
#
# XFB is the app store for this thing: the desktop serves whatever sits beside
# it as xfb-companion.apk, and the sidecar xfb-companion.json is what makes the
# phone treat a newer one as an update rather than a fresh install. Both have to
# be there, and the numbers in the sidecar have to be the APK's own — hence this
# script rather than a note in a document.
#
# Usage: ./package-companion.sh [destination-directory]
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- gradle ------------------------------------------------------------------
# There is no wrapper jar in this repository; Gradle 8.2 lives in the wrapper
# cache. An explicit GRADLE wins, then the cache, then whatever is on PATH.
gradle="${GRADLE:-}"
if [ -z "$gradle" ]; then
    gradle="$(ls -d "$HOME"/.gradle/wrapper/dists/gradle-8.2-bin/*/gradle-8.2/bin/gradle 2>/dev/null | head -1 || true)"
fi
if [ -z "$gradle" ]; then
    gradle="$(command -v gradle || true)"
fi
if [ -z "$gradle" ]; then
    echo "No Gradle 8.2 found. Set GRADLE=/path/to/gradle." >&2
    exit 1
fi

if [ ! -f "$here/keystore.properties" ]; then
    echo "No android/keystore.properties — the APK would come out unsigned and" >&2
    echo "Android would refuse to install it. See android/RELEASING.md." >&2
    exit 1
fi

echo "Building the release APK with $gradle"
"$gradle" --offline --console=plain -p "$here" assembleRelease

# --- what came out -----------------------------------------------------------
out="$here/app/build/outputs/apk/release"
metadata="$out/output-metadata.json"
[ -f "$metadata" ] || { echo "No $metadata; the build produced nothing." >&2; exit 1; }

read -r apk version code <<EOF
$(python3 - "$metadata" <<'PY'
import json, sys
element = json.load(open(sys.argv[1]))["elements"][0]
print(element["outputFile"], element["versionName"], element["versionCode"])
PY
)
EOF

apk="$out/$apk"
[ -f "$apk" ] || { echo "No $apk." >&2; exit 1; }

# --- signed? -----------------------------------------------------------------
# Checked rather than assumed: an unsigned APK installs nowhere, and finding
# that out on somebody's phone after they have downloaded it is the worst
# possible moment.
sdk="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
apksigner="$(ls "$sdk"/build-tools/*/apksigner 2>/dev/null | sort -V | tail -1 || true)"
if [ -n "$apksigner" ]; then
    if ! "$apksigner" verify "$apk" >/dev/null 2>&1; then
        echo "$apk is not signed. Check android/keystore.properties." >&2
        exit 1
    fi
    echo "Signed, and apksigner is happy with it."
else
    case "$apk" in
        *unsigned*) echo "$apk is unsigned." >&2; exit 1 ;;
        *) echo "No apksigner found; taking the file name's word for it." ;;
    esac
fi

# --- where XFB looks ---------------------------------------------------------
if [ $# -ge 1 ]; then
    destination="$1"
elif [ "$(uname)" = "Darwin" ]; then
    destination="$HOME/Library/Application Support/Netpack - Online Solutions/XFB"
else
    destination="$HOME/.local/share/Netpack - Online Solutions/XFB"
fi

mkdir -p "$destination"
cp "$apk" "$destination/xfb-companion.apk"
cat > "$destination/xfb-companion.json" <<EOF
{"versionName": "$version", "versionCode": $code}
EOF

echo
echo "xfb-companion.apk $version (versionCode $code)"
echo "  -> $destination"
echo
echo "Options > Sync to Phone should now say it is offering that file."
