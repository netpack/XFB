# Maintainer: Netpack <info@netpack.pt>
pkgname=xfb
pkgver=2.0.0
pkgrel=1
pkgdesc="Professional radio automation software with comprehensive accessibility support"
arch=('x86_64' 'aarch64')
url="https://netpack.pt"
license=('GPL3')
depends=(
    'qt6-base'
    'qt6-multimedia'
    'qt6-webengine'
    'qt6-quickwidgets'
    'at-spi2-core'
    'speech-dispatcher'
    'alsa-lib'
    'libpulse'
    'sqlite'
    'curl'
    'gstreamer'
    'gst-plugins-base'
    'gst-plugins-good'
)
makedepends=(
    'cmake'
    'git'
    'pkg-config'
)
optdepends=(
    'orca: Screen reader support for visually impaired users'
    'brltty: Braille display support'
    'espeak-ng: Text-to-speech synthesis'
    'audacity: Advanced audio editing'
    'yt-dlp: Download media from online sources'
    'ffmpeg: Audio format conversion'
    'lame: MP3 encoding'
    'sox: Audio processing'
    'flac: FLAC audio support'
    'vorbis-tools: OGG Vorbis support'
    'mp3gain: MP3 volume normalization'
    'normalize: Audio normalization'
    'wavpack: WavPack audio support'
    'opus-tools: Opus audio support'
    'mediainfo: Media file information'
)
source=("git+https://github.com/netpack/XFB.git#tag=v${pkgver}")
sha256sums=('SKIP')

build() {
    cd "$srcdir/XFB"
    
    cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCPACK_GENERATOR=""
    
    cmake --build build
}

check() {
    cd "$srcdir/XFB/build"
    # Run tests if available
    ctest --output-on-failure || true
}

package() {
    cd "$srcdir/XFB"
    
    # Install the main executable
    install -Dm755 "build/bin/XFB" "$pkgdir/usr/bin/xfb"
    
    # Install desktop file
    install -Dm644 "XFB.desktop" "$pkgdir/usr/share/applications/xfb.desktop"
    
    # Install icon
    install -Dm644 "xfb_icon.png" "$pkgdir/usr/share/pixmaps/xfb.png"
    
    # Install documentation
    install -Dm644 "README.md" "$pkgdir/usr/share/doc/$pkgname/README.md"
    
    # Install accessibility documentation
    if [ -d "docs/accessibility" ]; then
        cp -r "docs/accessibility" "$pkgdir/usr/share/doc/$pkgname/"
    fi
    
    # Install config directory structure
    install -dm755 "$pkgdir/usr/share/$pkgname/config"
    if [ -d "config" ]; then
        cp -r config/* "$pkgdir/usr/share/$pkgname/config/" 2>/dev/null || true
    fi
    
    # Install jingles directory if exists
    if [ -d "jingles" ]; then
        install -dm755 "$pkgdir/usr/share/$pkgname/jingles"
        cp -r jingles/* "$pkgdir/usr/share/$pkgname/jingles/" 2>/dev/null || true
    fi
}
