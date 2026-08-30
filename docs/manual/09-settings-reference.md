# 9. Settings reference

Most settings are in one window: **Options → Options**. Save with **Save
Settings** or **Close (and save)**; **Close (without saving)** discards.

Some changes apply the moment you save (theme, accent, font size, cue routing,
432 Hz, loudness); a few only take full effect after a restart, and the dialog
says so.

## General

**Look**

- **Theme** — System, Light, Dark, Midnight, Studio
- **Accent** — the highlight/selection/slider colour, with **Default** to go back
  to the theme's own
- **Application font size**
- **Language** — English, Portuguese, French
- **Start in FullScreen**

**Tabs**

- **XFB Torrents** — show the Torrents tab (you are asked to confirm the first
  time; see [chapter 6](06-listeners-and-streaming.md#torrents))
- **Audio FX tab** — show the equalizer and compressor tab
- **Pads tab** — show the cart wall. Hiding it keeps the pads

**Playback**

- **Disable the Seek Bar** — the presenter cannot scrub
- **Disable the Volume Slider**
- **432 Hz playback (all players)** — real-time retune from A=440; files are not
  modified and the tempo is preserved
- **Auto Auto-mix: mix tracks as they are added to the playlist**
- **Auto Mode: follow each track with one at a similar BPM**, and **BPM
  tolerance** — how far apart two tempos may be and still match
- **Loudness normalisation (EBU R128)**, with **Target loudness** (-23 LUFS is
  the EBU broadcast reference, -16 suits streaming and most automation, -9 is as
  loud as anything should be asked to go) and a **True peak ceiling** in dBTP
- **Show volume level meter (L/R LEDs)** and its **position** — horizontal next
  to the volume slider, or vertical beside the right panel

## Cue and outputs

- **Main output** — the device the station's audio goes to
- **Cue output** — headphones. It must be a different device; if it is the same,
  cueing stays disabled rather than let an audition reach the air
- **Cue level**
- **Speak announcements into the cue** and **spoken countdown** — the clock in
  your ear, not the listener's

The window tells you plainly which of those three states you are in.

## Database

- **Selected database** and its version
- **Delete Music Table**, **Delete Jingles Table**, **Delete Pub Table** — under
  a "Careful!" heading, and they mean it. There is no undo, but there is
  yesterday's copy in the `backups` folder next to the database

## Recording and Paths

- **Default recording device**, **Codec**, **Container**
- **Default recording path**, **program path**, **music path**, **jingle path**

Those four paths are where XFB writes and where its file pickers start. Set them
before you start cataloguing, not after.

## Network

The legacy client/server link — see [chapter
7](07-other-computers.md#the-legacy-clientserver-link).

- **Enable Networking**, **Server URL**, **Port**, **User**, **Password**
- **Role** — Client or Server
- **Communications hour**
- **FTP local temp folder**, **TakeOver temp folder**

## System Resources

Diagnostics: `uname`, `pwd`, `free`, `df`, and a **Troubleshoot** section with
**Manually edit settings.conf** for the settings that have no control of their
own.

## Downloads (yt-dlp)

- **Downloaded audio format** — opus, ogg or mp3. XFB produces it with ffmpeg
  and falls back to the next best open format if the encoder is unavailable
- **Keep the original downloaded video file**
- **Embed the video thumbnail as cover art**
- **Embed metadata (artist/title)**
- **Spotify account (optional)** — Client ID and secret. Without them, Spotify's
  public pages only give up the first 100 tracks of a playlist

`yt-dlp` is installed into your home folder and updated before each download.

## Settings that live elsewhere

| Setting | Where |
|---|---|
| Hour clocks, and whether Auto Mode follows them | **Options → Hour Clocks…** ([chapter 5](05-hour-clocks.md)) |
| Rotation rules, per-track categories and dayparts | **Options → Rotation Rules…** |
| Dead-air thresholds and fallback | **Options → Dead-Air Watchdog…** |
| How long the as-run log is kept | **Options → As-Run Log…** |
| The public page, and whether requests are accepted | **Options → Listener Requests…** |
| Icecast mounts, and auto-start | **Options → Stream to Icecast…** |
| Phone, backup and production pairing and schedules | the three sync windows ([chapter 7](07-other-computers.md)) |
| Screen reader verbosity and timing | **Options → Accessibility Preferences…** |
| Panel layout, lock, artwork panel | the **View** menu |
| Random jingle cadence | the playlist panel |

A few things have no control at all and are only in `xfb.conf` — the Auto Mode
no-repeat window (`AutoModeNoRepeatCount`, 10 by default) is the one most worth
knowing about. Edit it with XFB closed.
