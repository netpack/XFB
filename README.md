<div align="center">
  <img src="https://netpack.pt/img/xfb-home2.webp" alt="XFB Screenshot" width="800"/>
  <h1>XFB - Radio Automation Software</h1>
  <p><strong>The Open-Source Solution for Radio Broadcasting</strong></p>

  <p>
    <a href="https://github.com/netpack/XFB/releases"><img src="https://img.shields.io/github/v/release/netpack/XFB?display_name=tag&sort=semver" alt="Latest Release"></a>
    <a href="https://aur.archlinux.org/packages/xfb"><img src="https://img.shields.io/aur/version/xfb" alt="AUR version"></a>
    <a href="https://github.com/netpack/XFB/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPL v3"></a>
  </p>
</div>

XFB is an open-source radio automation software developed by [Frédéric Bogaerts](https://www.researchgate.net/profile/Frederic-Bogaerts) at [Netpack Online Solutions](https://www.netpack.pt).

## Features

- **Database Management** — Catalog and organize music, jingles, advertisements, and programs
- **Automated Scheduling** — Create schedules for music playback, ad slots, and program airing
- **Drag & Drop Playlist** — Drag tracks from the music library directly into the playlist
- **Multi-Player** — Main player plus two auxiliary LP players for DJ mixing
- **Audio FX** — Per-player 10-band equalizer (with presets) and broadcast-style compressor, applied live (XFB → Audio FX, requires ffmpeg)
- **432 Hz Playback** — Retune everything from A=440 to A=432 in real time without touching your files, or batch-convert one track, a selection, or the whole library (Database menu / music table right-click)
- **Live Recording** — Record programs directly within the application
- **Streaming Client** — Listen to any Icecast/Shoutcast stream or .m3u/.pls playlist from within XFB (with automatic reconnect), e.g. to monitor your station's output
- **Torrent Search** — Search and download music via Tor-routed onion sites
- **Accessibility** — ORCA screen reader support, keyboard navigation, audio feedback
- **Cross-Platform** — Runs on macOS, Linux (Debian/Arch), and Windows

---

## Installation

### macOS

**Option A — Homebrew (recommended):**

```bash
brew install --cask netpack/xfb/xfb
```

This installs XFB.app into /Applications together with ffmpeg (required by the
audio FX engine). Upgrade later with `brew upgrade --cask xfb` — or simply use
the in-app update notification.

**Option B — Manual:**

1. Download `XFB-3.141592-macOS.dmg` from [GitHub Releases](https://github.com/netpack/XFB/releases)
2. Open the DMG file
3. Drag `XFB.app` into your **Applications** folder
4. Launch XFB from Applications (first launch: right-click → Open to bypass Gatekeeper)

**Requirements:** macOS 11.0 (Big Sur) or later, Apple Silicon

### Debian / Ubuntu

```sh
# Download and install the .deb package
sudo apt install ./xfb_3.141592-1_amd64.deb

# Or if dependencies are missing:
sudo dpkg -i xfb_3.141592-1_amd64.deb
sudo apt install -f
```

**Requirements:** Ubuntu 20.04+ or Debian 11+, Qt6 runtime libraries

### Arch Linux (AUR)

```sh
# Using yay
yay -S xfb

# Or using paru
paru -S xfb

# Or manually
git clone https://aur.archlinux.org/xfb.git
cd xfb
makepkg -si
```

### Windows

1. Download the installer from [GitHub Releases](https://github.com/netpack/XFB/releases):
   - Intel/AMD 64-bit: `XFB-3.141592-Setup.exe`
   - ARM64 (Windows on ARM, e.g. Snapdragon): `XFB-3.141592-arm64-Setup.exe`
2. Run the installer and follow the prompts
3. Launch XFB from the Start Menu or Desktop shortcut

**Requirements:** Windows 10 or later (64-bit Intel/AMD or ARM64)

---

## Uninstallation

### macOS

**Option A — Manual:**
- Drag `XFB.app` from Applications to the Trash
- Optionally remove config: `rm -rf ~/Library/Application\ Support/Netpack\ -\ Online\ Solutions/XFB`

**Option B — Script:**
```sh
./uninstall-macos.sh
```
This removes the app, configuration, cache, and Tor data. Your music library is preserved.

### Debian / Ubuntu

```sh
# Remove the application (keeps configuration)
sudo apt remove xfb

# Remove everything including configuration
sudo apt purge xfb
```

### Arch Linux

```sh
# Remove the package
sudo pacman -R xfb

# Remove with unused dependencies
sudo pacman -Rns xfb
```

User configuration in `~/.config/XFB/` is preserved. Remove manually if desired.

### Windows

- **Installer version:** Use "Add or Remove Programs" in Windows Settings, or run `Uninstall.exe` from the install directory
- **Portable version:** Run `uninstall-windows.bat` then delete the XFB folder

User data in `%APPDATA%\Netpack - Online Solutions\XFB` is preserved.

---

## Usage

### Adding Music to the Playlist

1. Add music to the database via **File → Add a single song** or **File → Add all songs in a folder**
2. Browse your music library in the **Musics** tab at the bottom
3. **Drag and drop** a track from the music table onto the **Playlist** tab
4. Alternatively, right-click a track and select "Add to Playlist"

### Playback

- Click **Play** to start playback from the playlist
- Use the **Auto Mode** button to enable automatic advancement through the playlist
- The progress slider shows current position; the volume slider controls output level

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Play/Pause | Space |
| Stop | Ctrl+S |
| Next Track | Ctrl+Right |
| Full Screen | F11 |
| Add Music | Ctrl+M |
| Save Playlist | Ctrl+Shift+S |

---

## Building from Source

### Prerequisites

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- Qt6 (Core, Gui, Widgets, Multimedia, Sql, Network, WebEngineCore, WebEngineQuick, QuickWidgets)

### macOS

```sh
brew install qt@6 cmake
git clone https://github.com/netpack/XFB.git
cd XFB
./build-macos.sh
```

### Linux (Debian/Ubuntu)

```sh
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev qt6-webengine-dev libsqlite3-dev
git clone https://github.com/netpack/XFB.git
cd XFB
./build-deb-no-tests.sh
```

### Windows

```bat
REM Requires: Qt6 (MSVC), CMake, Visual Studio 2022 Build Tools

REM x64 (default)
set QT_DIR=C:\Qt\6.8.3\msvc2022_64
build-windows.bat

REM ARM64 (Windows on ARM). Best built natively on an ARM64 machine.
set QT_DIR=C:\Qt\6.8.3\msvc2022_arm64
build-windows.bat --arch arm64
```

To cross-compile ARM64 on an x64 machine, install the ARM64 MSVC build tools
plus both an x64 and an ARM64 Qt, then point `QT_HOST_DIR` at the x64 Qt so
`windeployqt` can run while deploying the ARM64 libraries:

```bat
set QT_DIR=C:\Qt\6.8.3\msvc2022_arm64
set QT_HOST_DIR=C:\Qt\6.8.3\msvc2022_64
build-windows.bat --arch arm64
```

---

## Optional Dependencies

For full functionality, consider installing:

| Tool | Purpose |
|------|---------|
| `tor` | Anonymous torrent search via onion sites |
| `aria2c` | Torrent downloading |
| `orca` | Screen reader support (Linux) |
| `exiftool` | Automatic track duration detection |
| `yt-dlp` | Download media from online sources |
| `ffmpeg` | Audio format conversion |

---

## Support

- [Open an issue](https://github.com/netpack/XFB/issues) on GitHub
- Email: info@netpack.pt
- Website: [netpack.pt](https://netpack.pt)

## License

XFB is licensed under the **GNU General Public License v3.0**. See the `LICENSE` file for details.

---

Made with ❤️ by Frédéric Bogaerts @ Netpack - Online Solutions
