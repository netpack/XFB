# 3. Going on air

## The running order

The **PlayList** tab is what XFB will play, top to bottom. Everything about
going on air comes back to getting the right things into it in the right order.

**Adding tracks**

- Select a track in any library table and press **Enter** — it goes on the end
- **Ctrl+Shift+Enter** / **Ctrl+Alt+Enter** — end / top of the running order
- Drag a track from a library table onto the PlayList tab
- Right-click a track → **Add to the bottom / top of the playlist**

**Reordering and removing** — right-click a track in the running order for:

- **Remove this track from the playlist**
- **Send this track to the top / bottom of the playlist**
- **Cue this track in the headphones**
- **Add a volume line** — a volume envelope over the track, for ducking music
  under a link or riding a long intro. Once set, the same menu offers
  **Reset the volume line** and **Remove the volume line**
- **Auto-mix the transition into this track**
- **Voice track over the join above this track…**

From the keyboard, **Ctrl+Shift+↑** and **Ctrl+Shift+↓** move the selected
track one place, and the new position is announced.

The panel shows the running order's total time, and **Sum to Playlist Time**
adds up what you have selected — the quick way to answer "does this fill the
half hour?"

**Saving a running order** — **File → Playlists → Save Playlist** writes an XML
file; **Load Playlist** brings one back, **Clear Playlist** empties the tab.
Saved playlists are also what the [dead-air watchdog](04-automation.md) falls
back to and what the phone companion can be pointed at.

## The transport

| Action | Shortcut |
|---|---|
| Play / Segue | Ctrl+Shift+P |
| Pause / Resume | Ctrl+Shift+Space |
| Stop | Ctrl+Shift+S |
| Next track | Ctrl+Shift+N |
| Previous track | Ctrl+Shift+B |
| Announce what is playing | Ctrl+Shift+W |
| Announce time remaining | Ctrl+Shift+R |
| Announce the intro countdown | Ctrl+Shift+I |
| Cue the selected track | Ctrl+Shift+C |
| Stop the cue | Ctrl+Shift+X |
| Voice track over this join | Ctrl+Shift+V |

They are all in the **Playback** menu, they work whatever has focus, and on
macOS you press Cmd instead of Ctrl.

**Play / Segue** is one button on purpose: pressed while something is playing,
it moves to the next item across the transition you have set up rather than
stopping and starting.

## Segues, wave view and Auto-mix

XFB preloads the next track and hands over to it, so a normal transition has no
gap in it.

**Wave view** draws each track's waveform in the running order. Drag a track
left to make it start before the previous one has finished, and you have set a
crossfade by eye.

**Auto-mix** works the overlap out for you, from where the outgoing track goes
quiet and where the incoming one starts, so each track begins where the last one
stops saying anything. Use it per track from the right-click menu, or tick
**Auto Auto-mix** in **Options → Options → General** to have every track mixed
as it is added to the running order.

Auto-mix reads the intro/outro measurements, so run **Database → Measure the
intro and outro times of the database** first if you want it at its best.

## Cue (pre-fade listen)

Cue plays a track into your headphones without any of it reaching the output.
It needs a second audio device, which is set up in **Options → Options → Cue and
outputs**:

- **Main output** — where the station's audio goes
- **Cue output** — headphones, and it must be a *different* device. If both are
  set to the same device, cueing stays switched off rather than risk an
  audition going to air
- **Cue level** — the monitor level in the headphones
- Optionally, **spoken announcements** and a **spoken countdown** into the cue
  ear only, which is how a blind operator hears the clock without the listener
  hearing it

**Ctrl+Shift+C** cues whatever is selected, **Ctrl+Shift+X** stops it, and while
something is cued the status bar says so and keeps saying so until it ends.

## Voice tracking

**Ctrl+Shift+V**, or right-click a track in the running order → **Voice track
over the join above this track…**, records your link across the join between two
songs and writes the ducking for you.

The dialog shows the join as a timeline — the end of the outgoing song, your
link, the start of the incoming one — and lets you set:

- **Input** — which recording device to use
- **Starts before the outgoing song ends** — where the link sits, in
  milliseconds, relative to the join
- **Music under the voice** — how loud the music stays while you are talking, as
  a percentage, and the **fade down and up** time

Press **Record**, talk, and stop. **Retake** throws the take away and starts
again. You can monitor on the main output while recording, or in the cue
headphones if a cue device is configured.

What you end up with is a volume envelope written onto both songs and your
recorded link between them — nothing needs mixing live when it airs.

## Pads (the cart wall)

The **Pads** tab is a grid of labelled, coloured buttons that play the instant
you press one, without going near the running order. It is built for a touch
screen.

- **Fill a pad** by dragging a track onto it, or turn on **Edit pads**
  (or right-click a pad) and choose **Library…** to search the database or
  **File…** to take any file on disk
- **Press to play, press again to stop** — or set the pad to restart from the
  top instead, which is what a stab wants
- **Loop until stopped** keeps a bed running under a live link
- Pads are polyphonic — a stinger over a bed is fine. **Stop all** cuts
  everything
- **Eight banks** hold separate sets; pads playing on one bank keep playing
  while you work in another
- Each pad has its own volume, label and colour, and everything is saved as you
  go

From the keyboard: **Tab** into the grid, **arrow keys** to move, **Enter** to
play or stop, **Esc** to stop, **F2** to edit the pad you are on. The whole grid
is a single Tab stop, so tabbing past it does not walk you through every pad.

## The DJ decks

The **DJ** tab is two auxiliary players, independent of the main one, each with:

- **Filter** — one knob: left for low-pass, right for high-pass, centre off
- **Echo** — dry through to a full echo-out tail
- **Brake** — the platter spins down to a stop
- **Backspin** — whips the record backwards, then stops

## Audio FX

The **Audio FX** tab (or **Options → Audio FX** when the tab is hidden) gives
each player — main, LP1 and LP2 — its own:

- **10-band equalizer** with presets and a preamp
- **Compressor** with threshold, ratio, attack, release and makeup gain

Both are applied live. The FX engine needs `ffmpeg`; without it the tab tells
you so and the controls are disabled.

**432 Hz playback** in **Options → Options → General** retunes everything from
A=440 to A=432 in real time, on all three players. Files are not modified and
the tempo is unchanged — track lengths and BPM stay the same.

**Loudness normalisation (EBU R128)**, also in General, plays every measured
track at the same programme loudness, with a true-peak ceiling and a limiter on
the master so a mis-measured track cannot clip the transmitter. Tracks that have
not been measured play untouched.

## Recording a programme

**Options → Client → Record a new Program** asks for a name. The format matters:
`NAME_YYYY-MM-DD`, for example `breakfast_2026-08-30` — the date is how XFB and
the station server tell one edition from the next.

Record, then press **Stop and process**. XFB asks whether to save it, adds it to
the Programs table, and — if this machine is configured as a client — uploads it
to the station server.

The recording device, codec, container and the folder each kind of file is
written to are in **Options → Options → Recording and Paths**.

**Options → Client → Make a program from this playlist** does the same job from
the other end: it renders whatever is in the running order into a single
programme file.
