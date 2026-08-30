# 8. Accessibility

XFB is built to be run without a mouse and without sight. This chapter is the
short version; there is a longer, Linux/ORCA-specific guide under
[`docs/accessibility/user-guide/`](../accessibility/user-guide/README.md),
including a [keyboard reference](../accessibility/user-guide/keyboard-reference.md)
and [troubleshooting](../accessibility/user-guide/troubleshooting.md).

## Screen readers

XFB speaks through whichever screen reader you already run — **ORCA** on Linux,
**VoiceOver** on macOS, **NVDA** on Windows. It does not launch one and does not
replace one. On Linux the spoken announcements go out through
speech-dispatcher, so a working speech-dispatcher is what makes XFB audible
there.

Braille displays are supported through **BrlTTY**.

## The tutorial

**Help → Tutorial for Blind Users**, or **Ctrl+Shift+H**.

It is a non-modal window, so you can try each step in the main window while it
stays open. If you are handing XFB to an operator who works by ear, start them
here rather than with this manual.

## Working from the keyboard

Every transport command has an application-wide shortcut, listed in the
**Playback** menu and in [chapter 3](03-going-on-air.md#the-transport). They work
whatever currently has focus.

- Inside the music, jingles, adverts and programmes tables, **Enter** adds the
  track you are on to the end of the running order. That is the whole
  running-order workflow in one key.
- **Tab** and **Shift+Tab** move between panels; **arrow keys** move within a
  table or the running order.
- Tab always *leaves* the control you are in — including multi-line text boxes
  in dialogs, where it moves to the next field rather than inserting a tab.
- **Ctrl+Shift+↑ / ↓** move the selected track in the running order, and the new
  position is announced.
- In the **Pads** grid, the whole grid is a single Tab stop: **Tab** in, arrow
  keys to move, **Enter** to play or stop, **Esc** to stop, **F2** to edit.

If you only remember three: **Enter** to build the running order,
**Ctrl+Shift+P** to go on air, **Ctrl+Shift+W** to hear what is playing.

## Hearing what is happening

| | |
|---|---|
| **Ctrl+Shift+W** | Announce what is playing |
| **Ctrl+Shift+R** | Announce time remaining |
| **Ctrl+Shift+I** | Announce the intro countdown — how long you can still talk before the vocal |

The intro countdown is drawn on the wave strip, which is no use at all to an
operator working by ear, which is why it has a key of its own.

Announcements can also be routed to the **cue output only**, so the clock and
the countdown reach your headphones and not the listener — see
[cue](03-going-on-air.md#cue-pre-fade-listen).

## Accessibility preferences

**Options → Accessibility Preferences…**, or **Ctrl+Shift+A**.

- **Enable accessibility features** — the master switch
- **Verbosity** — Terse (control type and name), Normal, or Verbose (with hints
  and descriptions)
- **Timing** — Immediate, Delayed (with a configurable delay in milliseconds, to
  avoid a pile-up), or On Request only
- **Interrupt announcements for critical alerts**
- **Announce tooltip content**
- **Announce state changes** — checked, expanded, and so on
- **Include time remaining when announcing what is playing** — one utterance
  instead of two, which matters because a screen reader given two announcements
  in quick succession routinely drops the first

## What is deliberately spoken

XFB announces the things an operator cannot see happening:

- a track moved in the running order, and its new position
- a fixed item queued by an [hour clock](05-hour-clocks.md), and when one is due
  with no audio behind it
- the cue starting and stopping
- dead-air incidents
- the result of a sync

If something important happens silently, that is a bug worth reporting.
