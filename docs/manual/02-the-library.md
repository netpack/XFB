# 2. The library

Everything XFB can put to air is in its library first. The library is four
separate catalogues, shown as the four lower tabs, and XFB treats them
differently:

| Tab | Holds | Used by |
|---|---|---|
| **Musics** | the music library | Auto Mode, rotation, the hour clock's music sweeps |
| **Jingles** | idents, sweepers, stabs, beds | the random-jingle cadence, pads, hour-clock jingle and station-ID slots |
| **Pub** | advertisements | the scheduler, hour-clock ad breaks |
| **Programs** | recorded or delivered shows | the scheduler, hour-clock programme and news slots |

A track only ever lives in one of them. If you want a jingle available to Auto
Mode as if it were music, add it to Musics as well — nothing shares.

## Adding music

**Database → Add a single song** takes one file and asks for the fields XFB
schedules on:

- **Artist** and **Song** — used for display, for search, and for the artist
  and title separation in the [rotation rules](04-automation.md#rotation-rules)
- **Genre 1** — the one Auto Mode and the hour clock match on. Genre 2 is
  recorded and searchable but does not drive scheduling
- **Country** — whether the track counts as national or international
- **Published date** — used to work out how much new music you are playing

**Database → Add all songs in a folder** catalogues a whole folder at once. It
can **include subfolders**, however deeply they nest, and follows a shortcut
once. Leave the artist field empty and XFB reads artist and title from the file
name, expecting `artist - song.ext`; the genre, country and published date you
set are applied to everything it finds.

**Database → Add a song from an external source** downloads with `yt-dlp` and
catalogues the result in one step. Paste a link and press **Get it!**, or paste
a YouTube playlist (a link containing `list=`) or a SoundCloud set and press
**Get Playlist!** to take every entry. The audio format, whether to keep the
original video, and whether to embed artwork and metadata are in
**Options → Options → Downloads (yt-dlp)**.

Spotify and Apple Music links are resolved to a track list and then fetched
from a source that can actually be downloaded from. Spotify's public pages only
expose the first 100 tracks of a playlist; to take longer ones in full, register
a free app at developer.spotify.com and paste its Client ID and secret into the
Downloads tab.

## Genres

**Database → Manage genres** edits the two genre lists. Genre 1 is the
scheduling genre — it is what an hour of the [hour grid](04-automation.md) or a
[music sweep](05-hour-clocks.md) names, so keep that list short enough to
programme with. Genre 2 is free.

Both add-music dialogs have a **Manage genres** button, so you can create a
genre without leaving the dialog.

## Jingles, adverts and programmes

**Database → Add a jingle** wants a file and a name. The name matters more than
it looks: it is what an [hour-clock slot](05-hour-clocks.md) matches on when it
asks for one specific ident rather than any jingle.

**Database → Add a publicity** and **Database → Add a program** take a name and
a file, and then offer **Schedule Options** — the times at which XFB will put
the item to air by itself. See [scheduled adverts and
programmes](04-automation.md#scheduled-adverts-and-programmes) for what each
kind of schedule does.

Programmes can also be recorded inside XFB or delivered to it over the network;
both are covered in [chapter 3](03-going-on-air.md#recording-a-programme) and
[chapter 7](07-other-computers.md).

## Finding things

The side panel's **Search** page searches the music table by artist or title.
The **filters** page narrows the table by Genre 1 and Genre 2.

Inside any of the four tables:

- **Enter** adds the track you are on to the end of the running order. This is
  the quickest way to build a show.
- **Ctrl+Shift+Enter** and **Ctrl+Alt+Enter** add to the end and to the top.
- **Ctrl+Shift+C** cues the selected track into the headphones without touching
  the output (see [cue](03-going-on-air.md#cue-pre-fade-listen)).
- **Right-click** gives the same actions plus the ones below.

### The right-click menu

- **Add to the bottom / top of the playlist**
- **Cue this** — audition on the cue output only
- **Batch Edit** (when more than one row is selected) — set Genre 1, Genre 2,
  Artist or Country on every selected track at once
- **Convert to 432 Hz** — permanently retune the selected files
- **Sync this selection / everything to the phone**, and clear the selection
- **Open this in Audacity** — needs `audacity` installed
- **Retrieve metadata from file (mediainfo)**
- **Delete this track from database** — asks twice: first to confirm removing
  the entries, then whether to delete the files from the disk as well. Answer
  No to the second question to keep the audio and lose only the catalogue entry.

## Measuring the library

Four analysis passes in the **Database** menu fill in things XFB can use later.
Each one only looks at tracks that have not been measured, so running them again
after adding music is cheap. They all need `ffmpeg`.

- **Measure the BPM of all music tracks in the database** — the tempo. Needed
  before Auto Mode's *follow each track with one at a similar BPM* option can do
  anything.
- **Measure the intro and outro times of the database** — where the vocal
  starts and where the track goes quiet. Feeds the intro countdown
  (**Ctrl+Shift+I**) and Auto-mix.
- **Measure the loudness (EBU R128) of the database** — integrated loudness and
  true peak, so every track can play at the same programme loudness. Files are
  never modified or re-encoded. Switch the playback side on in
  **Options → Options → General → Loudness normalisation**.
- **Convert all musics in the database to 432 Hz tuning** — this one *rewrites
  files*. If you only want to hear A=432 without touching your library, use the
  **432 Hz playback** option instead.

## Maintenance

The **Database** menu also holds the housekeeping:

- **Check & Update Music Table Records** — re-reads durations and fixes entries
  whose file has moved within reach
- **Check database data and DELETE all invalid records without confirmation** —
  exactly what it says. Entries whose file no longer exists are removed. There
  is no undo, but there is yesterday's copy in `backups/`.
- **Remove duplicate songs (same artist and song name)** — keeps one record per
  artist/title pair
- **AutoTrim the silence from the start and the end of all music tracks** —
  rewrites files
- **Convert all musics in the database to mp3 / ogg / opus** — a whole-library
  re-encode, and a long job

The side panel's **Extras** page has a **Sound Converter** for one-off work, and
a **With the Selected…** menu that converts, trims or deletes just the tracks
you have picked.

## What XFB writes back

When a track finishes on air, XFB stamps its `last_played` time and increments
its play count. The **Update last played values** button on the playlist panel
does the same by hand.

Those counters are for your information. The rotation rules deliberately do not
use them — they read the [as-run log](04-automation.md#the-as-run-log), which
records what actually went to air rather than what was queued.
