#ifndef STREAMINGCATALOG_H
#define STREAMINGCATALOG_H

#include <QList>
#include <QString>

#include <functional>

// Spotify and Apple Music track listings.
//
// Neither service hands out the audio itself — both stream DRM-protected
// content that yt-dlp cannot touch. What they do publish, without any API key,
// is the *track list*: what a playlist, an album or a single link is made of.
// That is all XFB needs. The catalogue turns a pasted Spotify/Apple Music link
// into artist/title pairs, and the downloader then fetches each of them the way
// it fetches everything else — through yt-dlp, via a YouTube search for that
// artist and title. So "download this Spotify playlist" means "get me these
// songs", not "rip Spotify".
//
// Resolution is synchronous and blocking (one HTTP request per link), so call
// it from the download worker thread, never from the UI thread.
namespace StreamingCatalog {

enum class Service {
    None,
    Spotify,
    AppleMusic
};

struct Track {
    QString artist;
    QString title;
};

struct Listing {
    Service service = Service::None;
    bool collection = false;   // an album/playlist rather than one track
    QString name;              // album/playlist name (empty for a single track)
    QList<Track> tracks;
    QString error;             // non-empty when the link could not be read

    // A long playlist must never be half-downloaded without saying so. When the
    // service reports how many tracks it holds, `expectedTotal` carries that
    // number (0 when it is unknown) and `truncated` says the list came back
    // short; `truncationNote` explains why and what would fix it.
    int expectedTotal = 0;
    bool truncated = false;
    QString truncationNote;
};

/** Which service a pasted link belongs to (None for YouTube/SoundCloud/…). */
Service serviceOf(const QString &url);

/** Human name of the service, for messages. */
QString serviceName(Service service);

/**
 * True when the link points at an album or a playlist rather than one track.
 * Cheap: decided from the URL shape alone, no network access.
 */
bool isCollectionUrl(const QString &url);

/**
 * Read the link's track list. Blocking; `log` (optional) receives progress
 * lines for the downloader console. On failure the returned Listing carries a
 * user-readable `error` and no tracks.
 */
Listing resolve(const QString &url,
                const std::function<void(const QString &)> &log = {});

/**
 * The yt-dlp argument that fetches this track: a single-result YouTube search
 * for the artist and title.
 */
QString searchUrlFor(const Track &track);

} // namespace StreamingCatalog

#endif // STREAMINGCATALOG_H
