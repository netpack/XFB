# Cross-compilation overrides for Linux musl on macOS

QMAKE_CC = x86_64-linux-musl-gcc
QMAKE_CXX = x86_64-linux-musl-g++
QMAKE_LINK = x86_64-linux-musl-g++
QMAKE_LINK_SHLIB = x86_64-linux-musl-g++

QMAKE_AR = x86_64-linux-musl-ar cqs
QMAKE_RANLIB = x86_64-linux-musl-ranlib

QMAKE_INCDIR += /usr/local/include
QMAKE_LIBDIR += /usr/local/lib

# Remove macOS specific flags individually to avoid cross-compiler errors
QMAKE_CFLAGS -= -arch
QMAKE_CFLAGS -= arm64
QMAKE_CFLAGS -= -mmacosx-version-min=14.0
QMAKE_CFLAGS -= -stdlib=libc++

QMAKE_CXXFLAGS -= -arch
QMAKE_CXXFLAGS -= arm64
QMAKE_CXXFLAGS -= -mmacosx-version-min=14.0
QMAKE_CXXFLAGS -= -stdlib=libc++

QMAKE_LFLAGS -= -arch
QMAKE_LFLAGS -= arm64
QMAKE_LFLAGS -= -mmacosx-version-min=14.0
QMAKE_LFLAGS -= -stdlib=libc++
