# Contributing to XFB

Thank you for your interest in XFB! Contributions of all kinds are welcome:
bug reports, code, documentation, translations, packaging, and testing on the
platforms and radio setups we don't have access to.

## Reporting bugs

Open an issue at <https://github.com/netpack/XFB/issues> and include:

- Your OS and XFB version (*Help → About*, or the title bar).
- Steps to reproduce, what you expected, and what happened instead.
- If relevant, the application log (XFB writes rotating logs under its
  application-data directory, e.g. `~/Library/Application Support/…/logs` on
  macOS or the equivalent AppData/`.local/share` path on Windows/Linux).

For **security issues**, please do not open a public issue — contact the
maintainer privately via <https://netpack.pt>.

## Building from source

XFB is a Qt 6 / C++17 application built with CMake. FFmpeg is required at
runtime for the FX engine, waveforms, and artwork extraction.

**Linux (Debian/Ubuntu)** — the authoritative dependency list lives in
`Dockerfile.debian-build` (it is what builds the release packages, and what CI
runs). In short:

```sh
sudo apt install build-essential cmake pkg-config qt6-base-dev \
  qt6-multimedia-dev qt6-webengine-dev qt6-declarative-dev # …see the Dockerfile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target XFB --parallel
```

Alternatively, `./build-deb-docker.sh amd64|arm64` builds the `.deb` in Docker
without touching your system (works from macOS too).

**macOS** — `./build-macos.sh` produces the app bundle and DMG. Qt 6 and
FFmpeg via Homebrew.

**Windows** — `build-windows.bat x64|arm64` (expects Qt 6 and NSIS; see the
script header for details).

**Arch Linux** — the `PKGBUILD` at the repository root is the same one
published on AUR (`xfb`).

## Code layout

- `src/player.cpp` / `player.h` — main window and playback orchestration (the
  heart of the application).
- `src/audio/` — the FX engine (`FxEngine`, `FxDsp`, `FxPlayer`), waveform
  extraction/cache (`WaveformStore`).
- `src/PlaylistWaveView.*` — playlist wave view, crossfade preparation,
  volume automation lines, auto-mix.
- `src/ThemeManager.*`, `src/ArtworkStore.*`, `src/LevelMeter.*` — theming,
  track artwork, output metering.
- `src/services/` — search/download services and the service layer.
- `src/dialogs/` — auxiliary dialogs (audio FX, …).

## Pull requests

- Target the `main` branch and keep each PR focused on one change.
- Match the surrounding code style (Qt idioms, C++17); new sources are added
  to **both** `src/CMakeLists.txt` and `src/xfb.pro`.
- Do not commit generated or build files (`ui_*.h`, qmlcache output, `build/`,
  packaging output) — `.gitignore` covers them.
- Test your change in the running app on at least one platform and say in the
  PR what you tested.

By contributing you agree that your contributions are licensed under the
[GNU GPL v3](LICENSE), the license of the project.
