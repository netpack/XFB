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
