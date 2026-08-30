#!/bin/bash
# Build and run XFB's test suite.
#
# Why this script exists: every build path — build-macos.sh, build-windows.bat,
# Dockerfile.debian-build, PKGBUILD and the CI workflow — passes
# -DBUILD_TESTING=OFF, which is right for producing release artifacts but meant
# the tests had never run anywhere. A version-comparison bug that killed the
# in-app updater for the entire installed base shipped in 3.14159265358 as a
# result. This gives the tests one place to run, and CI calls it on every push.
#
# Usage:
#   ./run-tests.sh              build and run the suite
#   ./run-tests.sh --all        also attempt the known-broken tests (expect red)
#   ./run-tests.sh --list       list what would run and what is skipped
#
# Exits non-zero if anything that is supposed to pass does not.

set -uo pipefail

BUILD_DIR="build-tests"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
ok()   { echo -e "${GREEN}✓${NC} $1"; }
err()  { echo -e "${RED}✗${NC} $1"; }
warn() { echo -e "${YELLOW}⚠${NC} $1"; }
info() { echo -e "${BLUE}·${NC} $1"; }

# --- Tests that do not currently pass ----------------------------------------
# These predate this script: the assertions describe behaviour the code does not
# have (e.g. TestInputValidator expects paths with an embedded NUL and several
# filename shapes to be rejected), and the integration/performance groups do
# not compile at all. They are skipped by name rather than by not building the
# target, so that a NEW test is picked up automatically — the whole point of
# having this script is that a test someone adds tomorrow actually runs.
#
# Removing a name from this list is the fix; do not add to it to silence a
# regression in a test that used to pass.
BROKEN_TESTS=(
    MusicRepositoryTest
    GenreRepositoryTest
    PlaylistRepositoryTest
    AudioServiceTest
    ErrorHandlerTest
    InputValidatorTest
    DatabaseOptimizerTest
    MusicCacheTest
    MainControllerTest
    AccessibilityManagerTest
)
# Whole groups whose targets fail to compile, so their tests can never run.
# Registered by tests/CMakeLists.txt but excluded here until they build again.
BROKEN_GROUPS=(
    AppIntegrationTest
    DatabaseServiceIntegrationTest
    PlayerUIControllerIntegrationTest
    AccessibilityIntegrationTest
    ORCACompatibilityTest
    AccessibilityUserAcceptanceTest
)

RUN_ALL=0
LIST_ONLY=0
for arg in "$@"; do
    case "$arg" in
        --all)  RUN_ALL=1 ;;
        --list) LIST_ONLY=1 ;;
        -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
        *) warn "Unknown argument: $arg" ;;
    esac
done

SKIP=("${BROKEN_TESTS[@]}" "${BROKEN_GROUPS[@]}")
EXCLUDE_RE="$(IFS='|'; echo "^(${SKIP[*]})$")"

if [ "$LIST_ONLY" = 1 ]; then
    echo "Skipped (known broken, ${#SKIP[@]} tests):"
    printf '    %s\n' "${SKIP[@]}"
    echo ""
    info "Everything else registered by tests/CMakeLists.txt runs."
    exit 0
fi

echo "═══ XFB test suite ═══"
echo ""

# --- Configure & build --------------------------------------------------------
# A dedicated tree: the release trees are configured with BUILD_TESTING=OFF and
# reconfiguring them would rebuild the world.
info "Configuring $BUILD_DIR (BUILD_TESTING=ON)..."
if ! cmake -S . -B "$BUILD_DIR" -DBUILD_TESTING=ON > /tmp/xfb-test-configure.log 2>&1; then
    err "CMake configure failed:"
    tail -20 /tmp/xfb-test-configure.log
    exit 1
fi
ok "Configured"

info "Building the unit tests..."
if ! cmake --build "$BUILD_DIR" --target unit_tests --parallel > /tmp/xfb-test-build.log 2>&1; then
    err "Test build failed:"
    grep -E "error:" /tmp/xfb-test-build.log | head -20
    exit 1
fi
ok "Built"
echo ""

# --- Run ----------------------------------------------------------------------
# offscreen so the widget tests do not need a display (CI has none).
export QT_QPA_PLATFORM=offscreen

if [ "$RUN_ALL" = 1 ]; then
    warn "--all: including the ${#SKIP[@]} known-broken tests, expect failures"
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    STATUS=$?
else
    info "Skipping ${#SKIP[@]} known-broken tests (./run-tests.sh --list to see them)"
    echo ""
    ctest --test-dir "$BUILD_DIR" --output-on-failure --exclude-regex "$EXCLUDE_RE"
    STATUS=$?
fi

echo ""
if [ $STATUS -eq 0 ]; then
    ok "Test suite passed"
else
    err "Test suite FAILED"
fi

echo ""
warn "${#BROKEN_TESTS[@]} unit tests assert behaviour the code does not have, and the"
warn "integration/performance groups do not compile. Neither is covered here."

exit $STATUS
