# The XFB Manual

This is the operator's manual for XFB — what each part of the application is
for, how to set it up, and what it does when you are not watching it.

It documents XFB **3.1422**. Where a screen has changed since your version, the
menu names here are the ones the code puts on screen today.

## How to read it

If you have just installed XFB and want to be on air this evening, read
chapters 1, 2 and 3 in order. That is enough to run a live show.

If the station is meant to run itself overnight, read chapter 4 and then
chapter 5. Automation is the part of XFB that does things while nobody is
looking, so it is also the part most worth understanding before you trust it.

Everything else is a reference: read the chapter when you need the feature.

## Contents

| # | Chapter | What it covers |
|---|---------|----------------|
| 1 | [Getting started](01-getting-started.md) | First launch, where your data lives, the window and its panels, themes and language |
| 2 | [The library](02-the-library.md) | Music, jingles, adverts and programmes; genres, searching, analysis, maintenance |
| 3 | [Going on air](03-going-on-air.md) | The running order, the transport, segues and auto-mix, cue, voice tracking, pads, decks, recording |
| 4 | [Automation](04-automation.md) | Auto Mode, the hour grid, rotation rules, scheduled adverts and programmes, the dead-air watchdog, the as-run log |
| 5 | [Hour clocks](05-hour-clocks.md) | Programming the hour as a clock: slots, fixed timing, station IDs, assigning clocks to the week |
| 6 | [Listeners and streaming](06-listeners-and-streaming.md) | Streaming to Icecast, the streaming client, the public now-playing page and listener requests |
| 7 | [Other computers](07-other-computers.md) | The phone companion, station backup, production computers, the legacy client/server link |
| 8 | [Accessibility](08-accessibility.md) | Screen readers, keyboard operation, spoken announcements, braille |
| 9 | [Settings reference](09-settings-reference.md) | Every tab of the Options window, and the settings that live outside it |
| 10 | [When something goes wrong](10-troubleshooting.md) | The log, dependencies, the database, and the faults that come up most |

## The menu bar, once

Five of XFB's menus are worth learning by name, because the rest of this manual
refers to them constantly:

- **File** — open a file, save / load / clear the running order
- **Database** — everything that adds to or maintains the library
- **Playback** — the transport, and every keyboard shortcut XFB has
- **Options** — settings, and every station-wide feature: hour clocks, rotation
  rules, the as-run log, the dead-air watchdog, listener requests, streaming,
  and the three sync windows
- **View** — show, hide, lock and reset the panels
- **Help** — About, and the tutorial for blind operators

The **Options** menu is the one people fail to find things in. It holds the
settings dialog *and* most of the station-wide features, and it is long. If
this manual says a feature is somewhere you cannot see, look there first.

## Conventions

- Menu paths are written **Options → Hour Clocks…**
- Shortcuts are written for Linux and Windows. On macOS press **Cmd** wherever
  the text says Ctrl.
- Where XFB does something on its own — queues a track, fires an ident, falls
  back to evergreen music — this manual says so plainly, and says what stops it.
