# XFB Dependencies for Arch Linux

This document maps the Debian/Ubuntu dependencies to their Arch Linux equivalents.

## Comparison Table

| Debian/Ubuntu Package | Arch Linux Package | Purpose |
|----------------------|-------------------|---------|
| **Build Dependencies** |
| `cmake` | `cmake` | Build system |
| `pkg-config` | `pkg-config` | Package configuration |
| `qt6-base-dev` | `qt6-base` | Qt6 core libraries |
| `qt6-multimedia-dev` | `qt6-multimedia` | Qt6 multimedia support |
| `qt6-webengine-dev` | `qt6-webengine` | Qt6 web engine |
| `qt6-quickwidgets-dev` | `qt6-quickwidgets` | Qt6 quick widgets |
| `libatspi2.0-dev` | `at-spi2-core` | Accessibility toolkit |
| `libspeechd-dev` | `speech-dispatcher` | Speech synthesis |
| `libasound2-dev` | `alsa-lib` | ALSA sound library |
| `libpulse-dev` | `libpulse` | PulseAudio library |
| `libsqlite3-dev` | `sqlite` | SQLite database |
| `libcurl4-openssl-dev` | `curl` | HTTP client library |
| **Runtime Dependencies** |
| `libqt6core6` | `qt6-base` | Qt6 core (included in qt6-base) |
| `libqt6gui6` | `qt6-base` | Qt6 GUI (included in qt6-base) |
| `libqt6widgets6` | `qt6-base` | Qt6 widgets (included in qt6-base) |
| `libqt6multimedia6` | `qt6-multimedia` | Qt6 multimedia runtime |
| `libqt6sql6` | `qt6-base` | Qt6 SQL (included in qt6-base) |
| `libqt6network6` | `qt6-base` | Qt6 network (included in qt6-base) |
| `libqt6webenginecore6` | `qt6-webengine` | Qt6 web engine core |
| `libqt6webengine6-data` | `qt6-webengine` | Qt6 web engine data |
| `libqt6quick6` | `qt6-declarative` | Qt6 Quick runtime |
| `libqt6qml6` | `qt6-declarative` | Qt6 QML runtime |
| `libatspi2.0-0` | `at-spi2-core` | AT-SPI runtime |
| `libspeechd2` | `speech-dispatcher` | Speech dispatcher runtime |
| `libasound2` | `alsa-lib` | ALSA runtime |
| `libpulse0` | `libpulse` | PulseAudio runtime |
| `libsqlite3-0` | `sqlite` | SQLite runtime |
| `libcurl4` | `curl` | cURL runtime |
| `gstreamer1.0-plugins-base` | `gst-plugins-base` | GStreamer base plugins |
| `gstreamer1.0-plugins-good` | `gst-plugins-good` | GStreamer good plugins |
| `gstreamer1.0-alsa` | `gst-plugins-base` | GStreamer ALSA (included) |
| `gstreamer1.0-pulseaudio` | `gst-plugins-good` | GStreamer PulseAudio (included) |
| **Accessibility Tools** |
| `orca` | `orca` | Screen reader |
| `speech-dispatcher` | `speech-dispatcher` | Speech synthesis dispatcher |
| `at-spi2-core` | `at-spi2-core` | AT-SPI core |
| `brltty` | `brltty` | Braille display support |
| `espeak-ng` | `espeak-ng` | Text-to-speech engine |
| **Audio Tools** |
| `audacity` | `audacity` | Audio editor |
| `ffmpeg` | `ffmpeg` | Audio/video converter |
| `lame` | `lame` | MP3 encoder |
| `sox` | `sox` | Audio processor |
| `flac` | `flac` | FLAC codec |
| `vorbis-tools` | `vorbis-tools` | OGG Vorbis tools |
| `mp3gain` | `mp3gain` | MP3 normalizer |
| `normalize-audio` | `normalize` | Audio normalizer |
| `wavpack` | `wavpack` | WavPack codec |
| `opus-tools` | `opus-tools` | Opus codec tools |
| `mediainfo` | `mediainfo` | Media file analyzer |
| `pulseaudio-utils` | `pulseaudio` | PulseAudio utilities |
| `alsa-utils` | `alsa-utils` | ALSA utilities |

## Installation Commands

### Minimal Installation (Required Dependencies Only)

```bash
sudo pacman -S --needed \
    qt6-base \
    qt6-multimedia \
    qt6-webengine \
    qt6-quickwidgets \
    at-spi2-core \
    speech-dispatcher \
    alsa-lib \
    libpulse \
    sqlite \
    curl \
    gstreamer \
    gst-plugins-base \
    gst-plugins-good
```

### Full Installation (With Accessibility Support)

```bash
sudo pacman -S --needed \
    qt6-base \
    qt6-multimedia \
    qt6-webengine \
    qt6-quickwidgets \
    at-spi2-core \
    speech-dispatcher \
    alsa-lib \
    libpulse \
    sqlite \
    curl \
    gstreamer \
    gst-plugins-base \
    gst-plugins-good \
    orca \
    brltty \
    espeak-ng
```

### Complete Installation (All Optional Tools)

```bash
sudo pacman -S --needed \
    qt6-base \
    qt6-multimedia \
    qt6-webengine \
    qt6-quickwidgets \
    at-spi2-core \
    speech-dispatcher \
    alsa-lib \
    libpulse \
    sqlite \
    curl \
    gstreamer \
    gst-plugins-base \
    gst-plugins-good \
    orca \
    brltty \
    espeak-ng \
    audacity \
    ffmpeg \
    lame \
    sox \
    flac \
    vorbis-tools \
    mp3gain \
    normalize \
    wavpack \
    opus-tools \
    mediainfo \
    pulseaudio \
    alsa-utils
```

## Package Size Comparison

Approximate installed sizes on Arch Linux:

| Category | Packages | Approximate Size |
|----------|----------|------------------|
| Core Qt6 | qt6-base, qt6-multimedia, qt6-webengine | ~200 MB |
| Accessibility | orca, speech-dispatcher, at-spi2-core | ~50 MB |
| Audio Tools | ffmpeg, sox, lame, etc. | ~100 MB |
| **Total (Full)** | All packages | ~350 MB |

## Notes

### Qt6 Packaging Differences

On Arch Linux, Qt6 is packaged differently than on Debian:

- **Debian**: Separate packages for each Qt6 module (libqt6core6, libqt6gui6, etc.)
- **Arch**: Consolidated packages (qt6-base includes core, gui, widgets, sql, network)

### GStreamer Plugins

On Arch Linux:
- `gst-plugins-base` includes ALSA support
- `gst-plugins-good` includes PulseAudio support
- No need for separate `gstreamer1.0-alsa` or `gstreamer1.0-pulseaudio` packages

### Audio Normalization

- Debian uses `normalize-audio`
- Arch uses `normalize` (same tool, different package name)

### PulseAudio vs PipeWire

Modern Arch installations may use PipeWire instead of PulseAudio:

```bash
# Check what you're using
pactl info

# If using PipeWire, install compatibility layer
sudo pacman -S pipewire-pulse
```

## Troubleshooting

### Missing Qt6 Modules

If you get errors about missing Qt6 modules:

```bash
# List all Qt6 packages
pacman -Ss qt6

# Install missing module
sudo pacman -S qt6-<module-name>
```

### Speech Dispatcher Not Working

```bash
# Start speech-dispatcher service
systemctl --user start speech-dispatcher

# Enable at login
systemctl --user enable speech-dispatcher

# Test speech
spd-say "Hello, this is a test"
```

### ORCA Not Announcing

```bash
# Ensure AT-SPI bus is running
systemctl --user start at-spi-dbus-bus

# Enable at login
systemctl --user enable at-spi-dbus-bus

# Start ORCA
orca &
```

### Audio Issues

```bash
# Check ALSA devices
aplay -l

# Check PulseAudio/PipeWire
pactl list sinks

# Test audio
speaker-test -c 2
```

## AUR Helper Installation

If you don't have an AUR helper yet:

### Install yay

```bash
sudo pacman -S --needed git base-devel
git clone https://aur.archlinux.org/yay.git
cd yay
makepkg -si
```

### Install paru

```bash
sudo pacman -S --needed git base-devel
git clone https://aur.archlinux.org/paru.git
cd paru
makepkg -si
```

## System Requirements

### Minimum

- Arch Linux (current)
- 2 GB RAM
- 500 MB disk space
- x86_64 or aarch64 processor

### Recommended

- 4 GB RAM
- 1 GB disk space (with all optional tools)
- Modern multi-core processor
- Sound card with ALSA/PulseAudio support

## See Also

- [AUR Publishing Guide](AUR_PUBLISHING_GUIDE.md)
- [Arch Wiki - Qt](https://wiki.archlinux.org/title/Qt)
- [Arch Wiki - Accessibility](https://wiki.archlinux.org/title/Accessibility)
- [Arch Wiki - PulseAudio](https://wiki.archlinux.org/title/PulseAudio)
