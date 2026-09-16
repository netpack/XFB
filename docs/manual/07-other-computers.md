# 7. Other computers

XFB can talk to three other kinds of machine, and they are deliberately separate
features with separate permissions. All three ride on the same small HTTP server
inside XFB, all three require pairing, and none of them encrypts anything: they
are meant for your own network, not the open internet.

| Feature | The other machine is | It may |
|---|---|---|
| **Sync to Phone** | the XFB companion app | read the library and playlists |
| **Station Backup** | a second XFB standing by | copy everything down, change nothing |
| **Production Computers** | an XFB used to prepare programme | copy down, *and* publish work back |

**Remote Control**, further down, is a different thing: not another XFB but any
program you point at the station, with its own port and keys instead of pairing.

## Sync to Phone

**Options → Sync to Phone…**

Serves this station's library and playlists to the XFB app on your phone.

- **Start serving** — or tick **Start serving when XFB starts**
- **Show a pairing code** — a six-digit code, also shown as a QR code the phone
  can scan. A phone that has not paired gets nothing
- **Reachable at** — the address to type into the phone if it cannot scan
- **Paired phones** — the list, and a button to remove one
- **Choose…** — which folder holds your saved playlists

If you put `xfb-companion.apk` in the folder the window names, XFB will offer
the app itself to a phone on the same network, which saves sideloading it by
hand.

Syncing is not encrypted. Pairing is what keeps strangers out.

## Station Backup

**Options → Station Backup…**

Keeps a second XFB on your network holding the same music, jingles, adverts,
programmes and schedule as the one on air — so that when the studio machine
dies, the standby is not a blank installation.

The same window is used on both machines, because when you are setting it up
nobody yet knows which will turn out to be which.

**On the machine that is on air** — the top half:

- **Start serving**, then **Pair a backup machine** for the six-digit code
- The paired machines are listed underneath

**On the backup machine** — the bottom half:

- **Station address** and **Port**, and the **pairing code** shown on the studio
  machine, then **Pair with this station**
- **Sync now**, and standing orders: **Sync as soon as XFB starts** and **every
  N minutes**
- **Check the studio every N seconds and warn after N seconds** — how long the
  studio may be dark before the backup says something

A sync **mirrors**: it makes this machine's catalogue match the station's. What
was removed there is removed here. That is the point of a backup, and it is why
you should not prepare programme on a backup machine — use a production computer
for that.

**Forget it** unpairs. Music already copied stays on the disk.

## Production Computers

**Options → Production Computers…**

Prepare the station's programme on another computer instead of on the one that
is broadcasting. The production machine copies down the station's music,
jingles, adverts, programmes and schedule, plays them locally while the work is
done, and publishes new or changed entries back — where Auto Mode picks them up
from its next choice onwards.

Again, one window, two halves.

**On the station** — let a production computer read the catalogue and add to it:
**Start serving**, **Pair a production computer**, and the list of those paired.
There is also **Also delete the audio file when a production computer withdraws
an entry**: with it off, a withdrawn advert or jingle only disappears from the
catalogue and its file stays on the disk.

**On the production machine** — **Station address**, **Port**, **pairing code**,
then:

- **Fetch from the station** — bring the catalogue down
- **Publish my work** — send new and changed entries up, with a count of what is
  waiting
- **Fetch as soon as XFB starts**, **Publish my work on every automatic run
  too**, and **every N minutes**

Fetching **merges** rather than mirrors: entries prepared here that the station
has not seen yet are not wiped by a fetch. That is the difference between this
and Station Backup, and it is why the two are separate features rather than one
with a switch.

## Remote Control

**Options → Remote Control…**

Lets another program drive the station over the network: a Stream Deck on the
presenter's desk, a home-automation panel, a script, or a web page of your own.
It is a separate server with its own port and its own keys — turning on phone
sync never opens it, and nothing listens until you tick **Accept remote
control**. Once ticked, it starts again every time XFB does.

- **Listen on** — **Every network connection**, or **This computer only** if the
  programs run on this machine or reach it through an SSH tunnel or TLS proxy
- **Port** — 8643 unless you change it
- **Keys** — **New key…** makes one. Give each program its own, so one can be
  revoked without breaking the others. A **Read only** key sees the status, the
  running order and the library; a **Control** key can also change what goes on
  air. The key is shown **once**: XFB keeps only a fingerprint of it, so a lost
  key is revoked and replaced, never recovered
- **Activity** — every command a key sends, and any address locked out for
  guessing

Whenever a key changes something, the status bar says so and names the key.
An address that presents ten wrong keys in five minutes is refused for five
minutes. The connection is **not encrypted**, like the other features in this
chapter.

### The control page

Most people should not need the API at all. XFB serves a small web page of its
own — **Also serve the control page** in that window, on by default — and the
window shows its address. Open it in any browser on the network, on a phone or
a laptop, sign in with a key, and you get a GUI: what is on air with its cover
and a playhead you can drag, Play, Pause, Stop, Next and Stop after, the
volume, Auto Mode, the recording and the stream, the running order (reorder or
remove a track, or clear it), and a library search that queues a track at the
end or plays it next.

- It is **the same API** underneath, so a key is what signs you in, and a
  read-only key gets the whole page with every control greyed out and a line
  saying why.
- The page updates itself as the station changes — the playhead, the track, the
  running order — without being reloaded.
- "Stay signed in on this device" keeps the key in that browser only. **Sign
  out** forgets it; do that on a shared computer.
- It loads nothing from the internet: no fonts, no frameworks, no analytics.
  Everything it needs — XFB's icon included — is in the page XFB serves, which
  is also why it works on a studio network with no internet at all. The one
  address that points outward is the donation link in the corner reminder, and
  nothing follows it unless somebody clicks it.
- Switch it off and its address answers 404 like any other unknown path, with
  the API carrying on as before.

### The API

Plain HTTP and JSON. Send the key on every request as
`Authorization: Bearer <key>`, and send arguments as a JSON object with
`Content-Type: application/json`. Every answer carries `"ok": true` or
`"ok": false` with an `"error"` sentence.

```bash
curl -H "Authorization: Bearer xfb_…" http://studio:8643/api/v1/status
curl -X POST -H "Authorization: Bearer xfb_…" http://studio:8643/api/v1/transport/next
curl -X POST -H "Authorization: Bearer xfb_…" -H "Content-Type: application/json" \
     -d '{"ref":"music:123","position":"start"}' http://studio:8643/api/v1/playlist/add
```

| Method and path | Key | Arguments | Does |
|---|---|---|---|
| `GET /api/v1/hello` | none | | Says it is XFB and which API version |
| `GET /api/v1/status` | read | | Transport, what is on air and time left, playlist length, Auto Mode, volume, recording, stream, whether the desk is locked |
| `GET /api/v1/playlist` | read | | The running order, with artist, title, duration and crossfade |
| `GET /api/v1/library` | read | `q`, `source` (`music`, `jingles`, `programs`), `limit` (up to 100) | Searches the library; each result has a `ref` such as `music:123` |
| `GET /api/v1/events` | read | | Server-sent events: the status straight away and again whenever it changes |
| `POST /api/v1/transport/play` | control | | Starts playing, resumes a pause, or cancels a stop-after |
| `POST /api/v1/transport/pause` | control | | Pauses |
| `POST /api/v1/transport/resume` | control | | Resumes |
| `POST /api/v1/transport/stop` | control | | Stops now |
| `POST /api/v1/transport/stop-after` | control | | Stops when the current track ends (the Play button's "Play and Stop") |
| `POST /api/v1/transport/next` | control | | Skips to the next track |
| `POST /api/v1/transport/seek` | control | `positionMs` | Moves the playhead, unless seeking is switched off in Options |
| `POST /api/v1/volume` | control | `volume` 0–100 | Sets the on-air volume, unless it is locked in Options |
| `POST /api/v1/automode` | control | `enabled` true/false | Switches Auto Mode |
| `POST /api/v1/playlist/add` | control | `ref`, `position` (`"start"`, `"end"` or an index) | Adds a library entry to the running order |
| `POST /api/v1/playlist/remove` | control | `index` | Removes one entry |
| `POST /api/v1/playlist/move` | control | `from`, `to` | Moves one entry, crossfade and volume line included |
| `POST /api/v1/playlist/clear` | control | | Empties the running order, without asking |
| `POST /api/v1/recording/start` | control | | Starts a programme recording after the usual five-second countdown |
| `POST /api/v1/recording/stop` | control | | Stops it |
| `POST /api/v1/stream/start` | control | | Puts the built-in Icecast stream on air |
| `POST /api/v1/stream/stop` | control | | Takes it off |

Tracks are only ever named by their library `ref`, never by a file path, so a
remote program can queue what the station already holds and nothing else.

Commands that are already in the state asked for succeed with
`"changed": false`. A command that cannot run says why with **409** — nothing to
play, nothing queued, the microphone not yet allowed on this computer, no
Icecast mount set up — rather than doing half of it. The other answers are
**401** no or wrong key, **403** a read key asked to change something, **404**
no such endpoint or entry, **422** a missing or malformed argument, and **429**
an address locked out.

`/api/v1/events` is a standard server-sent event stream, one `status` event per
change. A browser's own `EventSource` cannot send the `Authorization` header, so
a web page should read the stream with `fetch` instead.

## The legacy client/server link

XFB also has an older FTP-based arrangement, from before the sync features
above. It is configured in **Options → Options → Network**:

- **Enable Networking**, **Server URL**, **Port**, **User**, **Password**
- **Role** — Client or Server
- **Communications hour** — when a client calls in
- **FTP local temp folder** and **TakeOver temp folder**

A machine in the **Client** role uploads programmes it records to the server. A
machine in the **Server** role watches for delivered programmes and schedules
them. **Options → Server → Force FTP Check** and **Force monitoring** run those
checks now instead of waiting, and **Options → Update Dynamic Server's IP**
refreshes a dynamic DNS record.

Programme files delivered this way must be named `NAME_YYYY-MM-DD` — the date is
how the server tells one edition of a show from the next — and the programme
needs an hour and minute in the schedule for the server to be able to place it.

For anything new, prefer Station Backup and Production Computers. They are
pairing-based, they do not need an FTP server, and they are what is maintained.
