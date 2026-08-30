# 10. When something goes wrong

## Start with the log

XFB writes everything it does to **`xfb.log`**, in the same folder as
`xfb.conf`:

| | Path |
|---|---|
| macOS | `~/Library/Preferences/Netpack - Online Solutions/XFB/xfb.log` |
| Linux | `~/.config/Netpack - Online Solutions/XFB/xfb.log` |
| Windows | `%APPDATA%\Netpack - Online Solutions\XFB\xfb.log` |

It rolls when it passes 5 MB, keeping one previous generation, and anything
older than fourteen days is deleted — so it is the log for the problem you are
having now, not an archive. **Copy it out before doing much else**, and attach
it to any bug report.

## The faults that come up most

| Symptom | Cause and cure |
|---|---|
| **No FX, no waveforms, no BPM** | `ffmpeg` is missing. **Options → Install all dependencies**, or install it yourself |
| **Cue is disabled and will not turn on** | The main output and the cue output are the same device. Pick a second device in **Options → Options → Cue and outputs** |
| **Auto Mode adds nothing** | The library is empty, or the music table's paths all point at files that no longer exist. Run **Database → Check & Update Music Table Records** |
| **Auto Mode plays the same handful of tracks** | The pool it draws from is narrow — an hour's genre with little in it, or BPM matching on a library where few tracks are measured. Measure more, or widen the BPM tolerance |
| **An hour clock does nothing** | See the [hour-clock checklist](05-hour-clocks.md#if-a-clock-is-not-doing-what-you-expect). Nine times in ten it is either the Auto Mode tab or an ident left floating |
| **A scheduled advert never aired** | Check the rule is still in the list — a one-off deletes itself once it has aired, and a date interval deletes itself once its last day has passed ([chapter 4](04-automation.md#scheduled-adverts-and-programmes)) |
| **The dead-air watchdog never fires** | It measures from the level meter, which only runs while the FX engine is the active audio path. With passthrough playback there is nothing to measure ([chapter 4](04-automation.md#the-dead-air-watchdog)) |
| **Icecast refuses the connection** | Read the streaming log at the bottom of the stream window. If it is a protocol complaint, try toggling **Use HTTP PUT instead of SOURCE** |
| **A phone or backup will not pair** | The code expires — show a fresh one. Check both machines are on the same network and that the port is not firewalled |
| **Tracks vanished from the library** | Their files moved. XFB stores paths, not copies. Restore the files, or re-catalogue the folder |
| **The window layout is a mess** | **View → Reset the layout**, then **View → Lock the layout** so it stays put |

## The database

The library is a single SQLite file, `adb.db`, in the application data folder
([chapter 1](01-getting-started.md#where-your-data-lives)). XFB keeps a dated
copy in `backups/` each day it runs and keeps the last seven.

To go back a day: close XFB, rename the current `adb.db` out of the way, copy
`backups/adb-YYYYMMDD.db` into its place as `adb.db`, and start XFB.

Those backups sit on the same disk as the original, so they are protection
against a bad afternoon, not against a dead drive. Copy the folder somewhere
else on whatever schedule your station can live with.

## Running a second XFB on one machine

Set `XFB_PROFILE` before launching and you get a completely separate
installation — its own settings, its own library, its own caches — alongside the
normal one:

```bash
XFB_PROFILE=backup open -n /Applications/XFB.app
```

That is how to try **Station Backup** or **Production Computers** on one desk
before there is a second machine to put the other half on.

## Missing tools

XFB installs what it needs on demand and asks first. If you would rather have
everything ready in advance, **Options → Install all dependencies**.

| Tool | Needed for |
|---|---|
| `ffmpeg` | FX engine, waveforms, BPM, loudness, 432 Hz, format conversion (bundled on Windows and macOS) |
| `tor` | anonymous torrent search |
| `aria2c` or `transmission-cli` | torrent downloading |
| `yt-dlp` (with `node`) | downloading from online sources |
| `exiftool` | automatic track duration detection |
| `mediainfo` | the music table's "Retrieve metadata from file" action |
| `audacity` | the "Open this in Audacity" action |

`orca` is different: it is the screen reader *you* run, not something XFB
launches. Install it the usual way for your desktop.

## Reporting a bug

[Open an issue](https://github.com/netpack/XFB/issues) with:

- what you did, what you expected, what happened
- your XFB version (**Help → About**) and your operating system
- **`xfb.log`**, copied out as soon after the fault as possible
- for a crash, whatever crash report your system produced

Email `info@netpack.pt` if the issue tracker is not an option for you.
