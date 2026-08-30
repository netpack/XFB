# 6. Listeners and streaming

## Streaming to Icecast

**Options → Stream to Icecast…**

Sends what XFB is playing straight to an Icecast server, with the now-playing
metadata, from inside the application. No second encoder to run.

A **mount** is one destination, and you can configure several — a 128k web
stream and a low-bitrate mobile one, for instance. **Add**, **Duplicate** and
**Remove** manage the list; the panel on the right configures the selected one:

**Server**

- **Label** — your name for this mount
- **Host**, **Port**, **Mount point**, **User name**, **Password**
- **Use HTTP PUT instead of SOURCE** — Icecast 2.4.1 and later prefer PUT. If
  the server refuses the connection, try the other one
- **Codec** — MP3 (libmp3lame) or Opus (Ogg) — and **Bitrate**

**What listeners and directories see**

- **Station name**, **Genre**, **Description**, **Web site**
- **List this mount in public directories** — leave this off unless you want to
  be listed publicly

**Go on air automatically when XFB starts** does what it says, without anybody
opening this window.

**Save settings** stores the mount; **Go on air** connects. The streaming log at
the bottom is where a refused connection explains itself — read it there before
assuming the credentials are wrong.

## Listening to a stream

The side panel's **Streaming Client** page plays any Icecast/Shoutcast URL or
`.m3u`/`.pls` playlist, with its own volume and automatic reconnection. The
usual reason to use it is to monitor your own station's output as listeners
actually receive it.

## The streaming server page

The side panel's **Streaming Server** page drives external tools rather than
XFB's own encoder: a **StreamServer (butt)**, an **icecast2 web server**, a
**DDNS update**, a **port 8888 test**, and **Start All** / **Stop All** /
**Broadcast LIVE** to run them together.

**Public link (ngrok)** puts a stream that only exists on your own network onto
a public address: **Setup** configures your ngrok account once, **Share**
creates the link, **Copy** puts it on the clipboard.

This page assumes those tools are installed and is most at home on Linux. If you
just want to stream, the Icecast window above is the supported route.

## Listener requests

**Options → Listener Requests…**

Two switches, both off until you come here.

**Serve a public now-playing page on this network** puts a web page on this
computer's network address showing what is on air. Anyone who can reach this
machine can read it — on a home or studio LAN that means anyone on the network,
and if this machine is exposed to the internet, it means anyone at all. XFB
tells you the address the page can be reached at once it is serving.

**Let listeners search the library and ask for a track** adds the request form.
Listeners see **titles and artists only** — never a file, a path, or anything
else about your machine.

Give the page a **station name** (this computer's name is used if you leave it
empty) and an optional **tagline**.

**Nothing here plays by itself.** A request sits in the list until you do
something with it:

- **Add to the playlist** — puts the track at the end of the running order
- **Mark as played**
- **Dismiss**

The list shows when each request arrived, the track, who it is from and any
dedication, and you can show the ones already dealt with.

The page rides on the same server the phone and station sync use, so it shares
their port. See [chapter 7](07-other-computers.md).

## Torrents

The **Torrents** tab is hidden unless you switch it on in
**Options → Options → General → XFB Torrents**, and you are asked to confirm the
first time.

Searching runs through Tor over onion sites and is anonymised. **The BitTorrent
download itself is not** — your IP address is visible to the peers you download
from. XFB says so on the tab, and it is worth repeating here.

Connect to Tor first, then search; results can be downloaded, downloaded and
streamed, or have their magnet link copied. The Downloads section below the
results manages what is in flight.

Only download material you have the right to.
