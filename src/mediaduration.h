#ifndef MEDIADURATION_H
#define MEDIADURATION_H

#include <QString>

// Shared track-duration probe for the library add/check flows. Tries exiftool
// first (when installed), then falls back to ffmpeg (which the FX engine
// already requires and knows how to locate even with a minimal GUI PATH).
// Returns "h:mm:ss" — the format the musics.time column has always stored —
// or an empty string when the duration cannot be determined.
namespace MediaDuration {

QString forFile(const QString &filePath);

// True when at least one duration probe (exiftool or ffmpeg) is available;
// lets batch callers bail out early instead of failing per file.
bool probeAvailable();

// Parse a raw duration string as reported by exiftool ("0:03:45",
// "0:03:45.67", "3.07 s", possibly with "(approx)") or ffmpeg
// ("00:03:45.12"). Returns "h:mm:ss" or empty. Exposed for reuse/tests.
QString normalize(const QString &raw);

} // namespace MediaDuration

#endif // MEDIADURATION_H
