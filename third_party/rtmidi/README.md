# RtMidi

Upstream: https://github.com/thestk/rtmidi, release 5.0.0, by Gary P. Scavone.
Licence: MIT-style, in `LICENSE`.

`RtMidi.cpp`, `RtMidi.h` and `LICENSE` are unmodified copies from the 5.0.0
release tarball (`rtmidi_5.0.0.orig.tar.gz`, sha256
`c7923e4eee82b06c007435892cb2c3212d9007fa482c6b718943bda71c02c5a7`).

| file         | sha256 |
|--------------|--------|
| `RtMidi.cpp` | `dbd35cf313c5990553ec1a2e6718b428f6153bd43d0b457e695f7c8aa6454711` |
| `RtMidi.h`   | `a124f39855cb75875134f91d557e85cf33b177ef018112fe03ae6ae3f5b014ef` |

`CMakeLists.txt` is XFB's: it picks the platform's MIDI backend. To update,
replace the three upstream files with those of a newer release, record the
new checksums here and rebuild.
