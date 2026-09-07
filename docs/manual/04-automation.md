# 4. Automation

This chapter is about what XFB does when nobody is at the desk. Read it before
you leave a station running on its own overnight.

## Auto Mode

The green **Auto Mode** button is the switch. With it on, XFB keeps the running
order fed and moves through it on its own: when the queue runs short it picks
another track and adds it, and it keeps doing that indefinitely.

With it off, XFB plays exactly what you put in the running order and stops at
the end. Nothing in this chapter — not the scheduler, not the hour clock —
queues anything while Auto Mode is off, with the single exception of the
dead-air watchdog.

### How Auto Mode picks a track

For each pick, in this order:

1. **The hour's genre.** XFB looks up the genre programmed for this weekday and
   hour. If an [hour clock](05-hour-clocks.md) is running and its current music
   sweep names a genre, that genre is used instead.
2. **Three passes**, each dropping one constraint:
   - tempo-matched *and* within the hour's genre
   - within the hour's genre
   - the whole library

   A pass that comes back empty is not a failure; XFB moves to the next one. Only
   an empty library is a failure. This is why a programmed genre can never
   silence the station.
3. **The no-repeat window.** Tracks Auto Mode has played recently are excluded.
   The window is whatever `AutoModeNoRepeatCount` says (10 by default) but never
   more than half the library, so a small library cannot exclude everything it
   has.
4. **The rotation rules** choose between the candidates that survived. See below.
5. If every pass still came back empty — which means the exclusions ate the
   library — XFB repeats something rather than going silent. Repeating is the
   lesser fault.

**Tempo matching** is off unless you tick *Auto Mode: follow each track with one
at a similar BPM* in **Options → Options → General**, and it needs BPMs measured
(**Database → Measure the BPM of all music tracks in the database**). Half and
double time count as a match — a 140 BPM track drops over a 70 BPM one beat for
beat. The **BPM tolerance** setting decides how far apart two tempos may be.
Anything not yet measured is measured quietly in the background so the next pick
can use it.

### The two Auto Mode extras

On the playlist panel:

- **Random Jingle every N songs** — after every N music tracks, XFB drops a
  jingle from the Jingles table into the top of the running order. Set N to 0
  and the option switches itself off. It counts *records*, not minutes: for a
  jingle that has to land at a particular time — the pips, an ident that names
  the hour — use [time signals](05-hour-clocks.md#time-signals) instead.
- **Randomly add N songs** from a chosen genre — a one-shot button that tips a
  handful of tracks from one genre into the running order. Useful for
  hand-building a themed hour.

### The hour grid, honestly

XFB has an older mechanism — a genre per weekday and hour, and programmes pinned
to an hour and a minute — stored in the `hourgenre` and `hourprograms` tables.
Auto Mode still reads them, and they still sync between machines.

**There is no editor for them in XFB today.** Nothing in the application writes
to those tables; they can only be filled by editing the database directly. If
you want to programme the shape of your hours, use [hour
clocks](05-hour-clocks.md) — that is the supported way, it has a proper editor,
and where a clock has something to say it overrides the grid anyway.

## Rotation rules

**Options → Rotation Rules…**

A good picker is not a rotation. What a listener notices is the same artist
twice inside half an hour, the same song either side of the news, and a
Christmas record in June. That is what these rules are for.

They are **off by default**. Tick *Apply rotation rules when Auto Mode picks* to
turn them on; with it off, Auto Mode behaves exactly as it did before the
feature existed.

### Station rules

- **Same artist not within** — minutes before an artist may return (40 default)
- **Same title not within** — hours before a title may return (3 default)
- **Category weights** — how often Power, Secondary and Gold come up relative to
  one another, so the A-list turns over faster than the oldies
- **Default category** — what a track with no rules of its own counts as. Nobody
  categorises ten thousand tracks by hand, so this is what most of the library
  will use
- **Candidates per pick** — how large a random sample each pick chooses from

Both separations are measured against the **as-run log** — what actually went to
air — not against the running order and not against play counts.

### Per-track rules

The **Tracks** tab lists the library. Select one track or many, then set:

- **Category** — Power, Secondary or Gold
- **Hours the track may play** and **Weekdays the track may play** — dayparting
- **Not before** and **Expires after** — date windows, for seasonal material
- **Weight** — an override for this track, or leave it on the category's weight

Only the ticked rows are written, so you can set a category on a hundred tracks
without touching their dayparts. **Clear Rules** puts the selection back on the
station defaults.

### Nothing here is a hard filter

A station with 300 tracks and a 40-minute artist separation will, some quiet
night, have nothing left that satisfies every rule. An empty list is dead air, so
instead XFB relaxes the rules one at a time, in this fixed order, until
something survives:

1. everything honoured
2. artist separation halved
3. artist separation dropped
4. title separation halved
5. title separation dropped
6. daypart restrictions dropped
7. date windows dropped

The order runs from the least audible compromise to the most: hearing an artist
twice in twenty minutes is a shame, a Christmas record in June is a phone call,
so the seasonal window is the last thing to go.

Which step a pick needed is recorded. The **Why This Track?** tab shows the last
few Auto Mode picks and what each one cost. A rule that keeps having to be
relaxed is a rule that is wrong for your library — that tab is how you find out.

## Scheduled adverts and programmes

Adverts and programmes carry their own schedules, set in **Database → Add a
publicity** and **Database → Add a program** under **Schedule Options**. XFB
checks them once a minute and drops anything due into the **top** of the running
order.

Three kinds:

- **Date and time** — a one-off. It airs once at that minute and the rule is
  then deleted.
- **Day of the week and time** — every Monday at 08:30, and so on. It repeats
  until you delete the rule.
- **Date interval and time** — every day at that time, from one date to the
  other, both included. This is the campaign: *this advert, at ten past eight,
  for the fortnight it is paid for*. Once the last day has passed the rule
  removes itself.

When an advert is left with no rules at all — the one-off has aired, or the
campaign has finished — the advert itself is removed from the Pub table. That is
how a campaign ends without anybody remembering to end it. Programmes are never
removed this way; only their schedules are.

Add as many rules as you like to one advert: they all appear in the list at the
bottom of the dialog, and selecting one and pressing **Delete selected** removes
that rule and nothing else.

Scheduled items are queued at the top of the running order, not played over
whatever is on air. If a track is playing, the advert goes next.

## The dead-air watchdog

**Options → Dead-Air Watchdog…**

Silence is the one fault a station cannot survive quietly. The watchdog listens
to the output and, if it goes quiet for too long, puts something on.

**What counts as dead air** — a level below the **silence threshold** (in dBFS)
for longer than a set number of seconds. An incident ends after a set number of
seconds of healthy audio, and there is a quiet period afterwards so one bad
patch does not trigger a second rescue immediately. A loaded running order with
nothing playing can count as dead air too, if you tick that box.

Pressing Stop yourself never trips the watchdog.

**What to put on** — a saved XFB playlist (`.xml`), or a folder of evergreen
tracks, optionally shuffled, with a cap on how many tracks it queues.

**Telling somebody** — an on-screen alert, and an incident published for a
paired phone to collect. XFB's sync server is pull-only: it cannot ring your
phone, so the phone finds out next time it checks in.

**One thing to know before you rely on it.** Silence is measured from the level
meter, and the level meter only runs while the FX engine is the active audio
path. With plain passthrough playback there is nothing to measure, and only a
frozen transport will trip the watchdog. If the watchdog matters to you, run
with the FX engine active.

## The as-run log

**Options → As-Run Log…**

Every item XFB puts to air is logged: when it started, when it ended, what kind
of item it was, its artist and title, how long it was planned to run and how long
it actually ran, why it ended (played out, skipped, and so on), and what chose
it — you, the scheduler, Auto Mode.

The **Log** tab filters by date range, by kind, and by text in the artist, title
or file name, and exports what it lists as CSV. The **Advertiser report** tab
answers the question an advertiser actually asks: when did my spot run, and how
many times.

How long entries are kept is set at the bottom of the window.

The as-run log is also what the rotation rules read to decide what "recently"
means, so it is not only paperwork.
