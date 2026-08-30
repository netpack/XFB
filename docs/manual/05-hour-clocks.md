# 5. Hour clocks

**Options → Hour Clocks…**, or **Ctrl+Shift+K**

## Why a clock and not a grid

XFB's older scheduling is a table: one genre per hour, programmes pinned to an
hour and a minute. Radio does not plan an hour as a table. It plans it as a
*clock* — a pie, read clockwise from the top, with music sweeps, ad breaks,
jingle positions, news at :00 and the traffic at :20.

A clock says two things a grid cannot:

- **order**, so "sweep, jingle, sweep, ad break, news" is a fact about the hour
  rather than something the operator remembers; and
- **which items own their time**. News at the top of the hour must start at :00
  exactly — it is *hard-timed*. The sweep before it has no length of its own; it
  has "whatever is left until the news", and it is *floating*.

The feature is **off by default**, and while it is off it changes nothing: a
station that never opens the window has three unused database tables and
byte-identical behaviour.

## The window

Three tabs: **Clock** (build an hour), **Week** (say when it runs), **Auto Mode**
(switch it on).

### Clock — building an hour

Pick a clock from the drop-down at the top, or make one:

- **New…** — an empty clock
- **New from Example…** — a worked hour: news on the hour, sweeps either side of
  a jingle, ad break at twenty past. The fastest way to see how a clock behaves
- **Duplicate… / Rename… / Delete**

The hour is shown twice, and the two views are the same thing: a **wheel** you
can see the shape of, and a **list** with one row per item — Start, Length,
Type, Name, Genre or item, Timing. Everything the wheel can do is in the list,
so the whole feature works from the keyboard and through a screen reader.

Below the list are **Add slot**, **Remove slot**, **Move up** and **Move down**,
and below those the **Selected slot** box, which is where the actual editing
happens:

| Field | What it does |
|---|---|
| **Type** | What this slot puts on air — see the table below |
| **Name** | What you call it. Only for you: "Second sweep", "Traffic" |
| **Genre** / **Item** | What it draws from. The label changes with the type |
| **Fixed time** | Whether this item must start exactly at its own time |
| **Start past the hour** | Minutes and seconds from the top of the hour |
| **Length** | Minutes and seconds |

Change the fields, then press **Apply to slot**. Changes are not in the database
until you press **Save clock**; **Revert** throws away what you have not saved.

### The slot types

| Type | Puts on air | Draws from |
|---|---|---|
| **Music sweep** | a run of music | the library, through Auto Mode. The *Genre* field narrows it |
| **Advertisement break** | an advert | the **Pub** table |
| **Jingle** | a jingle | the **Jingles** table |
| **Programme** | a show | the **Programs** table |
| **News** | normally hard-timed at :00 | the **Programs** table |
| **Station ID** | the ident | the **Jingles** table |

For everything except a music sweep, the **Item** box lists the names in that
table, and you pick one. Leave it empty and XFB takes *any* row from that table
at random; type a name that matches nothing and you get the same random pick
rather than silence.

That fallback is deliberate — a programmed jingle should not become a hole — but
it has a consequence worth knowing: **Station ID and Jingle both draw from the
Jingles table**, so an ident slot with an empty or misspelled name will happily
play a sweeper. If you want *the* ident, name it exactly.

### Setting a station ID, start to finish

1. **Database → Add a jingle**: add the ident file and give it a name you will
   recognise, for example `Station ID`.
2. **Options → Hour Clocks… → Clock**, pick or create a clock, press **Add
   slot**.
3. Set **Type** to *Station ID*. The field below relabels itself from *Genre* to
   *Item*.
4. Pick your ident by name in **Item**.
5. Set **Start past the hour** and **Length** (idents are usually 5–15 seconds).
6. **Tick "Fixed time"** — see the warning below.
7. **Apply to slot**, then **Save clock**.
8. On the **Week** tab, assign the clock to the hours it should run.
9. On the **Auto Mode** tab, switch the feature on.

### Hard-timed and floating

This is the part worth understanding, because it decides what actually happens.

- A **hard-timed** slot always keeps its nominal start. Nothing around it can
  move it.
- **Floating** slots between two hard-timed anchors are laid end to end from the
  end of the earlier one. Spare room is absorbed by the **music sweeps** in that
  run, in proportion to their length — that is what "the sweep ends when the
  news is due" means numerically. If the run has no sweep, the last floating
  slot takes the slack.
- If the floating slots do **not** fit, nothing is silently shortened. They keep
  their lengths, they are marked as overrunning, and the window tells you by how
  much the hour is overfull. An hour that cannot work says so rather than
  quietly truncating something.
- Room nothing can absorb is reported as underfilled, again rather than pretended
  away.

The total line under the list is where you read all of this.

### Week — when each clock runs

The grid has hours down the side and weekdays across the top; press Return on a
cell to set it. Faster, use the **Assign** box below: pick a clock, pick days
(including *Every day*, *Monday to Friday*, *Saturday and Sunday*), pick a range
of hours, and press **Assign**. **Clear those hours** removes the assignment.

An hour with no clock assigned falls back to the old hour grid, which for most
stations means "no genre in particular".

### Auto Mode — switching it on

Nothing in this window affects what goes to air until you come here.

- **Let Auto Mode follow the hour clock** — Auto Mode fills the clock's current
  music sweep instead of using the hour grid's genre.
- **Put fixed-time items on air at their time** — the news, ad breaks and
  anything else marked fixed are queued when they come due.
- **Firing window** — how close to its nominal time an item counts as due, in
  seconds. The check runs every twenty seconds against a window of sixty by
  default, so an item is looked at three times inside its window; one missed
  tick during a busy segue still gets the news to air.

## What actually fires, and when it does not

Three conditions, all of them required:

1. **The feature is on** — both boxes on the Auto Mode tab.
2. **Auto Mode is running.** With the operator at the desk, nothing pushes items
   into the running order behind their back.
3. **The slot is hard-timed and is not a music sweep.**

That third one catches people out. A floating Station ID or jingle is never
queued — floating slots exist to shape the hour's timing and to tell Auto Mode
what to fill, not to fire. A **new slot is created floating**, so an ident you
add yourself does nothing until you tick *Fixed time*.

The worked example gets this right for you: its news, ident, jingle and both ad
breaks are hard-timed, and only the three music sweeps float.

Music sweeps are never fired either, and cannot be: there is no single file that
*is* a sweep. A sweep's job is to tell Auto Mode which genre to fill with.

When an item does fire, it goes to the **top** of the running order — next, not
last. A fixed item that lands after the four tracks Auto Mode has already queued
is not a fixed item at all. Each item fires once per day, hour and position, so a
long firing window cannot queue it twice.

If a programmed item comes due and its table has nothing in it, XFB says so once
— in the log and out loud — and does not retry every twenty seconds:

> *"Station ID is due now but there is no audio for it."*

## If a clock is not doing what you expect

| Symptom | Look at |
|---|---|
| Nothing at all happens | Both boxes on the **Auto Mode** tab, and whether Auto Mode itself is on |
| The music is right but idents never play | Are those slots ticked **Fixed time**? |
| The wrong jingle plays as the ident | The **Item** name must match a Jingles row exactly, or you get a random one |
| "…is due now but there is no audio for it" | The table that type draws from is empty |
| The hour is marked overfull | Something hard-timed is starting before the floating slots ahead of it can finish. Shorten them, or float what does not need to be fixed |
| A clock runs in the wrong hours | The **Week** tab; check the day columns as well as the hours |
