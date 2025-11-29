#!/bin/bash

# Build script for XFB project
# This script provides a convenient way to build the project with different configurations

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

echo_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

echo_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "XFB Build Script"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -d, --debug         Build in Debug mode (default: Release)"
    echo "  -t, --tests         Build and run tests"
    echo "  -a, --analysis      Run static analysis after build"
    echo "  -j, --jobs N        Number of parallel jobs (default: auto-detect)"
    echo "  --cmake-args ARGS   Additional CMake arguments"
    echo ""
    echo "Examples:"
    echo "  $0                  # Build in Release mode"
    echo "  $0 -d -t            # Build in Debug mode and run tests"
    echo "  $0 -c -a            # Clean build and run static analysis"
    echo "  $0 --cmake-args=\"-DCMAKE_INSTALL_PREFIX=/usr/local\""
}

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=false
RUN_ANALYSIS=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
CMAKE_ARGS=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -t|--tests)
            RUN_TESTS=true
            shift
            ;;
        -a|--analysis)
            RUN_ANALYSIS=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --cmake-args)
            CMAKE_ARGS="$2"
            shift 2
            ;;
        *)
            echo_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

echo_info "Starting XFB build process..."
echo_info "Build type: $BUILD_TYPE"
echo_info "Parallel jobs: $JOBS"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo_info "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo_error "CMake configuration failed!"
    exit 1
fi

# Build the project
echo_info "Building XFB..."
cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"

if [ $? -ne 0 ]; then
    echo_error "Build failed!"
    exit 1
fi

echo_success "Build completed successfully!"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo_info "Running tests..."
    ctest --output-on-failure --verbose -C "$BUILD_TYPE"
    
    if [ $? -eq 0 ]; then
        echo_success "All tests passed!"
    else
        echo_warning "Some tests failed. Check the output above."
    fi
fi

# Run static analysis if requested
if [ "$RUN_ANALYSIS" = true ]; then
    echo_info "Running static analysis..."
    cd "$PROJECT_ROOT"
    
    if [ -x "scripts/static_analysis.sh" ]; then
        ./scripts/static_analysis.sh
    else
        echo_warning "Static analysis script not found or not executable"
    fi
fi

echo_success "Build process completed!"
echo_info "Executable location: $BUILD_DIR/bin/ (or $BUILD_DIR/src/ depending on platform)"

# Show next steps
echo ""
echo_info "Next steps:"
echo "  - Run the application: $BUILD_DIR/bin/XFB (Linux/Windows) or $BUILD_DIR/src/XFB.app/Contents/MacOS/XFB (macOS)"
echo "  - Install: cmake --install $BUILD_DIR"
echo "  - Create package: cd $BUILD_DIR && cpack"
echo "  - Run tests: cd $BUILD_DIR && ctest"