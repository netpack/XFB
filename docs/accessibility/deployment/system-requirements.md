# XFB Accessibility System Requirements

## Overview

This document specifies the detailed system requirements for deploying XFB Radio Broadcasting Software with full accessibility support. These requirements ensure optimal performance and compatibility with assistive technologies.

## Operating System Requirements

### Supported Linux Distributions

**Tier 1 Support (Fully Tested and Supported):**
- Ubuntu 22.04 LTS (Jammy Jellyfish)
- Ubuntu 20.04 LTS (Focal Fossa)
- Debian 11 (Bullseye)
- Fedora 36, 37, 38

**Tier 2 Support (Compatible, Limited Testing):**
- openSUSE Leap 15.4+
- CentOS Stream 9
- Rocky Linux 9
- Arch Linux (current)
- Manjaro Linux (current)

**Minimum Kernel Version:** Linux 5.4 or newer
**Recommended Kernel Version:** Linux 5.15 or newer

### Desktop Environment Compatibility

**Fully Supported:**
- GNOME 40+ (Recommended for best ORCA integration)
- GNOME 3.38+ (Good compatibility)

**Partially Supported:**
- KDE Plasma 5.24+ (Basic accessibility support)
- XFCE 4.16+ (Limited accessibility features)
- MATE 1.26+ (Basic accessibility support)

**Not Recommended:**
- Lightweight window managers (i3, dwm, etc.)
- Wayland-only environments (limited AT-SPI support)

## Hardware Requirements

### Minimum Hardware Specifications

**Processor:**
- Architecture: x86_64 (AMD64)
- Speed: 2.0 GHz dual-core
- Features: SSE2 support required

**Memory:**
- System RAM: 4 GB minimum
- Available RAM for XFB: 2 GB minimum
- Swap space: 2 GB recommended

**Storage:**
- Available disk space: 2 GB for application
- Additional space for music library: Variable
- Temporary space: 1 GB for operations

**Audio Hardware:**
- Sound card: ALSA or PulseAudio compatible
- Sample rates: 44.1 kHz, 48 kHz support required
- Bit depth: 16-bit minimum, 24-bit recommended

### Recommended Hardware Specifications

**Processor:**
- Architecture: x86_64 (AMD64)
- Speed: 3.0 GHz quad-core or better
- Features: AVX2 support for audio processing

**Memory:**
- System RAM: 16 GB or more
- Available RAM for XFB: 8 GB or more
- Swap space: 4 GB on SSD

**Storage:**
- System drive: SSD with 10 GB available space
- Music library: Separate drive (SSD preferred)
- Network storage: Gigabit Ethernet for remote libraries

**Audio Hardware:**
- Professional audio interface (USB 2.0/3.0 or Firewire)
- Multiple input/output channels
- Low-latency drivers (JACK compatibility)
- Hardware monitoring capabilities

### Professional Broadcasting Hardware

**Audio Interfaces:**
- Focusrite Scarlett series
- PreSonus AudioBox series
- Behringer U-Phoria series
- RME Babyface series
- MOTU M series

**Microphones:**
- USB microphones with good Linux support
- XLR microphones with audio interface
- Headset microphones for monitoring

**Monitoring Equipment:**
- Studio monitors or professional headphones
- Audio level meters (hardware or software)
- Backup audio monitoring system

## Software Dependencies

### Core System Dependencies

**Qt Framework:**
```
qt6-base (>= 6.2.0)
qt6-multimedia (>= 6.2.0)
qt6-declarative (>= 6.2.0)
qt6-svg (>= 6.2.0)
qt6-tools (>= 6.2.0)
```

**Audio System:**
```
alsa-lib (>= 1.2.0)
pulseaudio (>= 14.0) OR pipewire (>= 0.3.40)
libsndfile (>= 1.0.28)
libsamplerate (>= 0.1.9)
```

**Database:**
```
sqlite3 (>= 3.35.0)
libsqlite3-dev
```

**Networking:**
```
libcurl (>= 7.68.0)
openssl (>= 1.1.1)
```

### Accessibility Dependencies

**AT-SPI Framework:**
```
at-spi2-core (>= 2.40.0)
at-spi2-atk (>= 2.38.0)
libatspi2.0-0 (>= 2.40.0)
libatspi2.0-dev (>= 2.40.0)
```

**Screen Reader Support:**
```
orca (>= 40.0) - Recommended: 42.0+
speech-dispatcher (>= 0.10.0)
espeak-ng (>= 1.50) OR festival (>= 2.5)
```

**Braille Display Support (Optional):**
```
brltty (>= 6.3)
liblouis (>= 3.17.0)
liblouisutdml (>= 2.9.0)
```

**Additional Accessibility Tools:**
```
python3-pyatspi (>= 2.40.0)
gir1.2-atspi-2.0 (>= 2.40.0)
```

### Development Dependencies (For Building from Source)

**Build Tools:**
```
build-essential
cmake (>= 3.16.0)
pkg-config
git
```

**Qt Development:**
```
qt6-base-dev
qt6-multimedia-dev
qt6-declarative-dev
qt6-svg-dev
qt6-tools-dev
```

**Additional Development Libraries:**
```
libasound2-dev
libpulse-dev
libspeechd-dev
libatspi2.0-dev
libsqlite3-dev
libcurl4-openssl-dev
```

## Network Requirements

### Local Network

**Bandwidth:**
- Minimum: 100 Mbps Ethernet
- Recommended: 1 Gbps Ethernet
- Wireless: 802.11n (2.4/5 GHz) minimum

**Latency:**
- Local network: < 1ms
- Internet streaming: < 50ms
- Remote library access: < 10ms

### Internet Connectivity (For Streaming)

**Upload Bandwidth:**
- MP3 128 kbps: 150 kbps minimum
- MP3 320 kbps: 400 kbps minimum
- FLAC streaming: 1.5 Mbps minimum

**Streaming Protocols:**
- Icecast2 support
- SHOUTcast support
- RTMP support (optional)

## Accessibility-Specific Requirements

### Screen Reader Compatibility

**ORCA Screen Reader:**
- Version: 40.0 minimum, 42.0+ recommended
- Speech synthesis: eSpeak-NG or Festival
- Braille support: BrlTTY integration
- Custom scripts: Python 3.8+ support

**Configuration Requirements:**
- AT-SPI enabled system-wide
- Accessibility toolkit support in Qt
- D-Bus session bus accessibility

### Keyboard and Input Requirements

**Keyboard Support:**
- Full 104-key keyboard recommended
- Function keys (F1-F12) required
- Numeric keypad recommended
- Modifier keys (Ctrl, Alt, Shift) essential

**Alternative Input Devices:**
- Switch access devices (via AT-SPI)
- Eye-tracking systems (with keyboard emulation)
- Voice recognition software compatibility

### Audio Output Requirements

**Speech Synthesis:**
- Multiple voice engines supported
- Adjustable speech rate (50-400 WPM)
- Punctuation level control
- Audio ducking support for music

**Audio Routing:**
- Separate audio channels for speech and music
- PulseAudio role-based routing
- JACK compatibility for professional setups
- Low-latency audio for real-time feedback

## Performance Requirements

### CPU Performance

**Minimum Performance:**
- Single-threaded: 2000 PassMark CPU score
- Multi-threaded: 4000 PassMark CPU score
- Audio processing: Real-time capability

**Recommended Performance:**
- Single-threaded: 3000+ PassMark CPU score
- Multi-threaded: 8000+ PassMark CPU score
- Audio processing: Professional-grade real-time

### Memory Performance

**RAM Usage Patterns:**
- Base application: 500 MB
- Music library (10,000 tracks): 200 MB
- Accessibility services: 100 MB
- Audio buffers: 50 MB
- Peak usage: 1.5x base requirements

**Memory Bandwidth:**
- Minimum: DDR3-1600
- Recommended: DDR4-2400 or better

### Storage Performance

**Disk I/O Requirements:**
- Random read: 100 IOPS minimum
- Sequential read: 50 MB/s minimum
- Database operations: Low-latency access
- Music file access: Sustained throughput

**SSD Benefits:**
- Faster application startup
- Reduced library scan times
- Better accessibility responsiveness
- Improved overall user experience

## Compatibility Matrix

### Operating System vs. ORCA Version

| OS Version | ORCA 40.x | ORCA 41.x | ORCA 42.x | ORCA 43.x |
|------------|-----------|-----------|-----------|-----------|
| Ubuntu 20.04 | ✓ | ✓ | ✓ | ⚠️ |
| Ubuntu 22.04 | ✓ | ✓ | ✓ | ✓ |
| Debian 11 | ✓ | ✓ | ✓ | ⚠️ |
| Fedora 36 | ✓ | ✓ | ✓ | ✓ |
| Fedora 37+ | ⚠️ | ✓ | ✓ | ✓ |

**Legend:**
- ✓ Fully supported and tested
- ⚠️ Compatible but may require additional configuration
- ✗ Not supported or incompatible

### Audio System Compatibility

| Audio System | XFB Support | Accessibility | Latency | Professional Use |
|--------------|-------------|---------------|---------|------------------|
| PulseAudio | ✓ Full | ✓ Excellent | Medium | Good |
| PipeWire | ✓ Full | ✓ Good | Low | Excellent |
| JACK | ✓ Limited | ⚠️ Basic | Very Low | Excellent |
| ALSA Direct | ⚠️ Basic | ⚠️ Limited | Low | Good |

## Validation Tools

### System Compatibility Check

**Automated Validation Script:**
```bash
#!/bin/bash
# XFB System Requirements Validation

echo "XFB System Requirements Check"
echo "============================="

# Check OS version
OS_VERSION=$(lsb_release -d | cut -f2)
echo "Operating System: $OS_VERSION"

# Check kernel version
KERNEL_VERSION=$(uname -r)
echo "Kernel Version: $KERNEL_VERSION"

# Check CPU
CPU_INFO=$(lscpu | grep "Model name" | cut -d: -f2 | xargs)
CPU_CORES=$(nproc)
echo "CPU: $CPU_INFO ($CPU_CORES cores)"

# Check memory
TOTAL_MEM=$(free -h | grep "Mem:" | awk '{print $2}')
AVAIL_MEM=$(free -h | grep "Mem:" | awk '{print $7}')
echo "Memory: $TOTAL_MEM total, $AVAIL_MEM available"

# Check disk space
DISK_SPACE=$(df -h / | tail -1 | awk '{print $4}')
echo "Available disk space: $DISK_SPACE"

# Check Qt version
QT_VERSION=$(qmake -query QT_VERSION 2>/dev/null || echo "Not found")
echo "Qt Version: $QT_VERSION"

# Check ORCA
ORCA_VERSION=$(orca --version 2>/dev/null | head -1 || echo "Not installed")
echo "ORCA Version: $ORCA_VERSION"

# Check AT-SPI
if systemctl --user is-active at-spi-dbus-bus.service >/dev/null 2>&1; then
    echo "AT-SPI: Active"
else
    echo "AT-SPI: Inactive or not available"
fi

# Check audio system
if command -v pulseaudio >/dev/null 2>&1; then
    if pulseaudio --check; then
        echo "Audio System: PulseAudio (running)"
    else
        echo "Audio System: PulseAudio (not running)"
    fi
elif command -v pipewire >/dev/null 2>&1; then
    echo "Audio System: PipeWire"
else
    echo "Audio System: Unknown or ALSA only"
fi

echo ""
echo "Validation complete. Check compatibility matrix for support status."
```

### Performance Benchmark

**XFB Performance Test:**
```bash
#!/bin/bash
# XFB Performance Benchmark

echo "XFB Performance Benchmark"
echo "========================"

# CPU benchmark
echo "Running CPU benchmark..."
CPU_SCORE=$(sysbench cpu --cpu-max-prime=20000 run | grep "events per second" | awk '{print $4}')
echo "CPU Score: $CPU_SCORE events/sec"

# Memory benchmark
echo "Running memory benchmark..."
MEM_SCORE=$(sysbench memory --memory-total-size=1G run | grep "transferred" | awk '{print $4}')
echo "Memory Throughput: $MEM_SCORE"

# Disk benchmark
echo "Running disk benchmark..."
DISK_SCORE=$(sysbench fileio --file-total-size=1G prepare && sysbench fileio --file-total-size=1G --file-test-mode=rndrw run | grep "read, MiB/s" | awk '{print $3}')
echo "Disk I/O: $DISK_SCORE MiB/s"
sysbench fileio cleanup

# Audio latency test
echo "Testing audio latency..."
if command -v jack_iodelay >/dev/null 2>&1; then
    AUDIO_LATENCY=$(timeout 5 jack_iodelay 2>/dev/null | tail -1 | awk '{print $1}' || echo "N/A")
    echo "Audio Latency: $AUDIO_LATENCY ms"
else
    echo "Audio Latency: Cannot test (JACK not available)"
fi

echo ""
echo "Benchmark complete. Compare results with minimum requirements."
```

## Upgrade Paths

### From Previous XFB Versions

**XFB 1.x to 2.x:**
- Backup existing configuration
- Update system dependencies
- Migrate accessibility settings
- Test ORCA compatibility

**System Upgrade Considerations:**
- Ubuntu 20.04 → 22.04: Full compatibility
- Debian 10 → 11: Requires dependency updates
- Fedora N → N+1: Usually seamless

### Hardware Upgrade Recommendations

**Priority Order for Performance:**
1. SSD storage (biggest impact)
2. Additional RAM (if < 8GB)
3. Faster CPU (for large libraries)
4. Professional audio interface
5. Network infrastructure

For specific hardware recommendations or compatibility questions, consult the XFB hardware compatibility database or contact technical support.