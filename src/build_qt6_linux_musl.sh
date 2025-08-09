#!/bin/bash
# Script to build Qt6 for Linux musl target on macOS using musl-cross toolchain

set -e

# Set variables
QT_VERSION=6.5.1
QT_SRC_DIR=$HOME/qt6-src
QT_BUILD_DIR=$HOME/qt6-build-linux-musl
MUSL_CROSS_PREFIX=x86_64-linux-musl-
SYSROOT=$HOME/musl-cross-sysroot

# Check if musl-cross toolchain is installed
if ! command -v ${MUSL_CROSS_PREFIX}gcc &> /dev/null
then
    echo "musl-cross toolchain not found. Installing via Homebrew..."
    brew tap filosottile/musl-cross
    brew install filosottile/musl-cross/musl-cross
fi

# Check if SYSROOT directory exists
if [ ! -d "$SYSROOT" ]; then
    echo "SYSROOT directory not found at $SYSROOT"
    echo "Please create or set up a sysroot for the Linux musl target."
    echo "You can create a sysroot by extracting a Linux musl root filesystem or using a tool like debootstrap."
    exit 1
fi

# Download Qt6 source if not present
if [ ! -d "$QT_SRC_DIR" ]; then
  echo "Downloading Qt6 source..."
  git clone --branch v$QT_VERSION https://code.qt.io/qt/qt5.git $QT_SRC_DIR
  cd $QT_SRC_DIR
  # Correct module subset names for Qt6 6.5.1
  perl init-repository --module-subset=qtbase,qtdeclarative,qtquickcontrols2,qttools,qtwebengine
else
  echo "Qt6 source directory already exists."
fi

# Create build directory
mkdir -p $QT_BUILD_DIR
cd $QT_BUILD_DIR

# Set environment variables for cross-compilation
export CC=${MUSL_CROSS_PREFIX}gcc
export CXX=${MUSL_CROSS_PREFIX}g++
export AR=${MUSL_CROSS_PREFIX}ar
export RANLIB=${MUSL_CROSS_PREFIX}ranlib
export STRIP=${MUSL_CROSS_PREFIX}strip
export SYSROOT=$SYSROOT

# Configure Qt6 for Linux musl cross-compilation
$QT_SRC_DIR/configure \
  -prefix /usr/local/qt6-linux-musl \
  -release \
  -opensource -confirm-license \
  -xplatform linux-clang-libc++ \
  -device-option CROSS_COMPILE=${MUSL_CROSS_PREFIX} \
  -sysroot $SYSROOT \
  -nomake tests -nomake examples \
  -skip qtwayland \
  -no-pch \
  -no-rpath \
  -no-compile-examples

# Build and install
make -j$(sysctl -n hw.ncpu)
make install

echo "Qt6 Linux musl build completed. Installed to /usr/local/qt6-linux-musl"
