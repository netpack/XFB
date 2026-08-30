# 1. Getting started

## Installing

Installation instructions for macOS, Debian/Ubuntu, Arch and Windows are in the
project [README](../../README.md#installation), and they change with each
release, so they are not repeated here. On macOS and Linux the package manager
route (`brew install --cask netpack/xfb/xfb`, `yay -S xfb`, `apt install`) is
worth preferring: it also keeps you on the upgrade path.

XFB does not need any optional tool installed in advance. The first time you use
something that needs one — the FX engine, waveforms, downloads, torrent search —
XFB tells you what it needs and why, asks, and installs it through your
platform's package manager. **Options → Install all dependencies** fetches the
lot in one go if you would rather have everything ready before a shift.

## The first launch

XFB creates its own library database the first time it starts. There is no
setup wizard and nothing to point it at: it copies an empty skeleton database
into your application data folder and opens it.

That database, `adb.db`, is the station. Every track, jingle, advert,
programme, schedule and rotation rule lives in it. **The audio files themselves
stay where they are** — XFB stores the path to each file, not a copy of it — so
moving or renaming your music folder after cataloguing it will break the
entries that point into it.

XFB keeps a dated copy of the database in a `backups` folder every day it is
run, and keeps the last seven. That is a safety net for XFB's own mistakes, not
a backup strategy: it sits on the same disk as the original.

## Where your data lives

| | Path |
|---|---|
| macOS | `~/Library/Application Support/Netpack - Online Solutions/XFB/XFB/` |
| Linux | `~/.local/share/Netpack - Online Solutions/XFB/XFB/` |
| Windows | `%APPDATA%\Netpack - Online Solutions\XFB\XFB\` |

The doubled `XFB` is not a typo: the outer folder holds things XFB downloads and
generates (the companion app, voice tracks, Tor data), and the inner one holds
the library — `adb.db` and `backups/`. When you are looking for the database,
it is the one in the *inner* folder.

Settings live in `xfb.conf`, in the configuration folder for your platform
(`~/.config/…` on Linux, `~/Library/Preferences/…` on macOS,
`%APPDATA%\…` on Windows). **`xfb.log` sits next to `xfb.conf`** — that is the
first file to look at when something goes wrong, and the first thing to attach
to a bug report. See [chapter 10](10-troubleshooting.md).

## The window

XFB's main window is made of four panels and two groups of tabs.

**The panels** can each be shown, hidden, dragged to another edge, or floated
off into their own window. They are listed in the **View** menu:

- **Player** — the transport, the progress bar, the volume slider and what is
  on air now
- **Clock** — the time, and the countdowns
- **Side panel** — search, filters, extras, and the streaming client and server
  (it also has its own show/hide arrow in the top right of the tab strip)
- **Library** — the music, jingles, adverts and programmes tables

Once the window looks the way you want it, tick **View → Lock the layout**.
That hides the panel title bars and stops a panel being dragged loose by a
mis-aimed click during a show — worth doing on any machine that goes on air.
**View → Reset the layout** puts everything back where it started.

**The upper tabs** are what you work in:

- **PlayList** — the running order (see [chapter 3](03-going-on-air.md))
- **HistoryList** — what has already played this session
- **DJ** — two auxiliary decks with filter, echo, brake and backspin
- **Pads** — the cart wall, if it is enabled
- **Audio FX** — the equalizer and compressor, if it is enabled

**The lower tabs** are the library: **Musics**, **Jingles**, **Pub**
(advertisements), **Programs**, and **Torrents** if you have switched it on.

## Making it yours

Everything here is in **Options → Options** unless noted.

- **Theme** — Light, Dark, Midnight, Studio, or System to follow your desktop.
  Applies as soon as you save.
- **Accent** — the colour used for highlights, selections and sliders. Each
  theme has its own default, and a **Default** button to go back to it.
- **Application font size** — applies immediately; a restart guarantees every
  view has picked it up.
- **Language** — English, Portuguese or French. Some strings only change after
  a restart.
- **Start in FullScreen**, and **Options → FullScreen** to toggle it now.
- **Show volume level meter (L/R LEDs)** — a stereo LED meter, either next to
  the volume slider or vertically beside the right-hand panel. It is fed by the
  FX playback engine, which matters for the [dead-air
  watchdog](04-automation.md#the-dead-air-watchdog).
- **Disable the Seek Bar** / **Disable the Volume Slider** — for a studio
  machine where a presenter should not be able to scrub or ride the output.
- **Audio FX tab**, **Pads tab**, **XFB Torrents** — show or hide those tabs.
  Hiding the Pads tab keeps the pads; they come back with it.
- **View → Artwork** shows or hides the artwork panel inside the side panel.

## Keeping up to date

**Options → Check for updates** asks the release server what the latest version
is and offers to fetch it. XFB also checks on its own and tells you in the
status bar when there is something new.

If you installed through Homebrew, apt, pacman or winget, upgrading through
that package manager works too and keeps the package database honest.
