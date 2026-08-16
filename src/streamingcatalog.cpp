#include "streamingcatalog.h"

#include "secretstore.h"

#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QtDebug>

namespace {

using Log = std::function<void(const QString &)>;

void say(const Log &log, const QString &line)
{
    qDebug().noquote() << "[StreamingCatalog]" << line;
    if (log)
        log(line);
}

// How many tracks Spotify's public embed page will list. Measured, not
// documented: a 150-track playlist comes back with exactly 100 and no hint
// that anything is missing.
const int kSpotifyEmbedLimit = 100;

// Both sites serve a different (or no) page to a client that doesn't look like
// a browser, so ask as one.
const char *kUserAgent =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

using Headers = QList<QPair<QByteArray, QByteArray>>;

// Blocking request. Runs its own event loop, so this must never be called on
// the UI thread — the download worker is where it belongs. Pass `postBody` to
// POST instead of GET.
QByteArray httpRequest(const QString &url, const Headers &headers,
                       const QByteArray *postBody, QString *error)
{
    QNetworkAccessManager nam;
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", kUserAgent);
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    for (const auto &header : headers)
        request.setRawHeader(header.first, header.second);

    QNetworkReply *reply = postBody
        ? nam.post(request, *postBody)
        : nam.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    // A stalled request must not hang the whole download queue.
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        reply->abort();
        loop.quit();
    });
    timeout.start(30000);
    loop.exec();

    QByteArray body;
    if (reply->error() == QNetworkReply::NoError) {
        body = reply->readAll();
    } else if (error) {
        *error = reply->errorString();
    }
    reply->deleteLater();
    return body;
}

QByteArray httpGet(const QString &url, QString *error)
{
    return httpRequest(url, {}, nullptr, error);
}

QJsonDocument parseJson(const QByteArray &body, QString *error)
{
    if (body.isEmpty())
        return {};

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (doc.isNull() && error)
        *error = parseError.errorString();
    return doc;
}

QJsonDocument httpGetJson(const QString &url, QString *error)
{
    return parseJson(httpGet(url, error), error);
}

QString collapseSpaces(QString s)
{
    static const QRegularExpression runs(QStringLiteral("\\s+"));
    return s.replace(runs, QStringLiteral(" ")).trimmed();
}

// --- Spotify --------------------------------------------------------------

// Accepts every shape Spotify hands out: open.spotify.com/track/<id>, the
// localized .../intl-pt/track/<id>, embed links, and spotify:track:<id> URIs.
bool spotifyParts(const QString &url, QString *kind, QString *id)
{
    static const QRegularExpression re(
        QStringLiteral("(track|album|playlist)[/:]([A-Za-z0-9]+)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(url);
    if (!m.hasMatch())
        return false;
    if (kind) *kind = m.captured(1).toLower();
    if (id)   *id = m.captured(2);
    return true;
}

// Join the artist names of a Spotify API object ("artists":[{"name":...}]).
QString spotifyArtists(const QJsonObject &item)
{
    QStringList names;
    const QJsonArray artists = item.value("artists").toArray();
    for (const QJsonValue &value : artists) {
        const QString name = value.toObject().value("name").toString();
        if (!name.isEmpty())
            names << name;
    }
    return collapseSpaces(names.join(QStringLiteral(", ")));
}

// The Spotify Web API credentials, if the operator registered an app and put
// them in xfb.conf. Without them XFB falls back to the embed page, which stops
// at 100 tracks — enough for most playlists, but not for a long one.
bool spotifyCredentials(QString *clientId, QString *clientSecret)
{
    const QString configPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/xfb.conf");
    QSettings settings(configPath, QSettings::IniFormat);
    *clientId = settings.value(QStringLiteral("SpotifyClientId")).toString().trimmed();
    // Stored sealed by the Options dialog, like the other saved password.
    *clientSecret = SecretStore::open(
        settings.value(QStringLiteral("SpotifyClientSecret")).toString()).trimmed();
    return !clientId->isEmpty() && !clientSecret->isEmpty();
}

// Client-credentials flow: an app token, no user login. Good for reading
// public playlists and albums.
QString spotifyAccessToken(const QString &clientId, const QString &clientSecret,
                           QString *error)
{
    const QByteArray basic =
        (clientId + QLatin1Char(':') + clientSecret).toUtf8().toBase64();
    const Headers headers = {
        {"Authorization", "Basic " + basic},
        {"Content-Type", "application/x-www-form-urlencoded"}
    };
    const QByteArray body = "grant_type=client_credentials";

    const QJsonObject response =
        parseJson(httpRequest(QStringLiteral("https://accounts.spotify.com/api/token"),
                              headers, &body, error), error)
            .object();

    const QString token = response.value("access_token").toString();
    if (token.isEmpty() && error && error->isEmpty())
        *error = response.value("error_description").toString();
    return token;
}

// Walk a paged Spotify collection to its end, 100 tracks at a time. This is the
// path that gets a long playlist complete rather than stopping at the embed's
// first hundred.
bool spotifyPagedTracks(const QString &endpoint, const QString &token,
                        StreamingCatalog::Listing *listing, const Log &log)
{
    const Headers headers = {{"Authorization", ("Bearer " + token).toUtf8()}};
    const int pageSize = 100;

    for (int offset = 0; ; offset += pageSize) {
        QString error;
        const QJsonObject page =
            parseJson(httpRequest(QStringLiteral("%1?limit=%2&offset=%3")
                                      .arg(endpoint).arg(pageSize).arg(offset),
                                  headers, nullptr, &error),
                      &error)
                .object();

        if (page.contains("error")) {
            const QJsonObject apiError = page.value("error").toObject();
            say(log, QStringLiteral("Spotify API: %1")
                         .arg(apiError.value("message").toString()));
            return false;
        }
        if (page.isEmpty()) {
            say(log, QStringLiteral("Spotify API request failed: %1").arg(error));
            return false;
        }

        if (offset == 0)
            listing->expectedTotal = page.value("total").toInt();

        const QJsonArray items = page.value("items").toArray();
        if (items.isEmpty())
            break;

        for (const QJsonValue &value : items) {
            QJsonObject item = value.toObject();
            // A playlist wraps each track; an album's track list does not.
            if (item.contains("track"))
                item = item.value("track").toObject();
            if (item.isEmpty())
                continue; // a removed track, or a podcast episode

            StreamingCatalog::Track track;
            track.title = collapseSpaces(item.value("name").toString());
            track.artist = spotifyArtists(item);
            if (!track.title.isEmpty())
                listing->tracks.append(track);
        }

        say(log, QStringLiteral("Spotify: read %1 of %2 track(s)...")
                     .arg(listing->tracks.size()).arg(listing->expectedTotal));

        if (page.value("next").isNull() || items.size() < pageSize)
            break;
    }
    return true;
}

// Read a Spotify link through the official Web API, which pages and so has no
// 100-track ceiling. Returns false when the API can't serve this link, so the
// caller can fall back to the embed.
bool resolveSpotifyViaApi(const QString &kind, const QString &id,
                          StreamingCatalog::Listing *listing, const Log &log)
{
    QString clientId;
    QString clientSecret;
    if (!spotifyCredentials(&clientId, &clientSecret))
        return false;

    QString error;
    const QString token = spotifyAccessToken(clientId, clientSecret, &error);
    if (token.isEmpty()) {
        say(log, QStringLiteral("Spotify API sign-in failed (%1) — falling back to the "
                                "embed page.").arg(error));
        return false;
    }

    const Headers headers = {{"Authorization", ("Bearer " + token).toUtf8()}};

    if (kind == QLatin1String("track")) {
        const QJsonObject item =
            parseJson(httpRequest(QStringLiteral("https://api.spotify.com/v1/tracks/%1").arg(id),
                                  headers, nullptr, &error), &error).object();
        const QString title = collapseSpaces(item.value("name").toString());
        if (title.isEmpty())
            return false;
        listing->tracks.append({spotifyArtists(item), title});
        listing->expectedTotal = 1;
        return true;
    }

    // Name the collection first; the track pages don't carry it.
    const QJsonObject entity =
        parseJson(httpRequest(QStringLiteral("https://api.spotify.com/v1/%1s/%2").arg(kind, id),
                              headers, nullptr, &error), &error).object();
    if (entity.contains("error")) {
        say(log, QStringLiteral("Spotify API could not open that %1 (%2) — falling back "
                                "to the embed page.")
                     .arg(kind, entity.value("error").toObject().value("message").toString()));
        return false;
    }
    listing->name = collapseSpaces(entity.value("name").toString());

    say(log, QStringLiteral("Reading the whole %1 through the Spotify API...").arg(kind));
    if (!spotifyPagedTracks(QStringLiteral("https://api.spotify.com/v1/%1s/%2/tracks")
                                .arg(kind, id),
                            token, listing, log)) {
        listing->tracks.clear();
        listing->expectedTotal = 0;
        listing->name.clear();
        return false;
    }
    return !listing->tracks.isEmpty();
}

// The embed page carries the entity — name, artists and, for albums and
// playlists, the whole track list — as a JSON blob. It needs no API key and no
// login, unlike the Web API. Its ceiling is 100 tracks.
StreamingCatalog::Listing resolveSpotify(const QString &url, const Log &log)
{
    StreamingCatalog::Listing listing;
    listing.service = StreamingCatalog::Service::Spotify;

    QString kind;
    QString id;
    if (!spotifyParts(url, &kind, &id)) {
        listing.error = QStringLiteral(
            "That does not look like a Spotify track, album or playlist link.");
        return listing;
    }
    listing.collection = (kind != QLatin1String("track"));

    // The official API pages through the whole thing; the embed stops at 100.
    // Prefer the API whenever credentials are configured, and fall back
    // silently when they are not (or when the API declines this link).
    if (resolveSpotifyViaApi(kind, id, &listing, log)) {
        say(log, QStringLiteral("Spotify: %1 track(s)%2.")
                     .arg(listing.tracks.size())
                     .arg(listing.name.isEmpty()
                              ? QString()
                              : QStringLiteral(" in \"%1\"").arg(listing.name)));
        if (listing.expectedTotal > listing.tracks.size()) {
            listing.truncated = true;
            listing.truncationNote =
                QStringLiteral("Spotify reports %1 tracks but only %2 could be read.")
                    .arg(listing.expectedTotal).arg(listing.tracks.size());
        }
        return listing;
    }

    const QString embedUrl =
        QStringLiteral("https://open.spotify.com/embed/%1/%2").arg(kind, id);
    say(log, QStringLiteral("Reading the Spotify %1...").arg(kind));

    QString error;
    const QByteArray page = httpGet(embedUrl, &error);
    if (page.isEmpty()) {
        listing.error = QStringLiteral("Could not reach Spotify: %1")
                            .arg(error.isEmpty() ? QStringLiteral("empty response") : error);
        return listing;
    }

    static const QRegularExpression nextData(
        QStringLiteral("<script id=\"__NEXT_DATA__\"[^>]*>(.*?)</script>"),
        QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = nextData.match(QString::fromUtf8(page));
    if (!m.hasMatch()) {
        listing.error = QStringLiteral(
            "Spotify did not return the track list for that link. Make sure it is "
            "public (a private or collaborative playlist can't be read).");
        return listing;
    }

    const QJsonObject entity = QJsonDocument::fromJson(m.captured(1).toUtf8())
                                   .object()
                                   .value("props").toObject()
                                   .value("pageProps").toObject()
                                   .value("state").toObject()
                                   .value("data").toObject()
                                   .value("entity").toObject();
    if (entity.isEmpty()) {
        listing.error = QStringLiteral("Spotify returned a page XFB could not read.");
        return listing;
    }

    listing.name = entity.value("name").toString();

    const QJsonArray trackList = entity.value("trackList").toArray();
    if (!trackList.isEmpty()) {
        for (const QJsonValue &value : trackList) {
            const QJsonObject item = value.toObject();
            StreamingCatalog::Track track;
            track.title = collapseSpaces(item.value("title").toString());
            track.artist = collapseSpaces(item.value("subtitle").toString());
            if (!track.title.isEmpty())
                listing.tracks.append(track);
        }
    } else {
        // A single track: the artists live beside the name.
        QStringList artists;
        const QJsonArray artistArray = entity.value("artists").toArray();
        for (const QJsonValue &value : artistArray) {
            const QString name = value.toObject().value("name").toString();
            if (!name.isEmpty())
                artists << name;
        }
        StreamingCatalog::Track track;
        track.title = collapseSpaces(listing.name);
        track.artist = collapseSpaces(artists.join(QStringLiteral(", ")));
        if (!track.title.isEmpty())
            listing.tracks.append(track);
        listing.name.clear();
    }

    if (listing.tracks.isEmpty()) {
        listing.error = QStringLiteral("That Spotify link holds no playable tracks.");
        return listing;
    }

    // The embed hands out at most 100 tracks and never says how many it held
    // back, so landing exactly on the ceiling is the only signal that a longer
    // playlist was cut short. Say so rather than quietly downloading a part.
    if (listing.tracks.size() == kSpotifyEmbedLimit) {
        listing.truncated = true;
        listing.truncationNote = QStringLiteral(
            "Spotify's public page hands out at most %1 tracks, so anything beyond "
            "that was not read. To download longer playlists in full, register a "
            "Spotify app (developer.spotify.com) and put its credentials in "
            "xfb.conf as SpotifyClientId and SpotifyClientSecret.")
            .arg(kSpotifyEmbedLimit);
    }

    say(log, QStringLiteral("Spotify: %1 track(s)%2.")
                 .arg(listing.tracks.size())
                 .arg(listing.name.isEmpty() ? QString()
                                             : QStringLiteral(" in \"%1\"").arg(listing.name)));
    return listing;
}

// --- Apple Music ----------------------------------------------------------

struct AppleParts {
    QString storefront = QStringLiteral("us"); // country segment, e.g. /pt/
    QString kind;                              // album | song | playlist | ...
    QString id;                                // trailing path id
    QString trackId;                           // ?i= — one track of an album
};

AppleParts appleParts(const QString &url)
{
    AppleParts parts;
    const QUrl parsed(url);
    const QStringList segments =
        parsed.path().split('/', Qt::SkipEmptyParts);

    for (int i = 0; i < segments.size(); ++i) {
        const QString &segment = segments.at(i);
        if (i == 0 && segment.size() == 2) {
            parts.storefront = segment.toLower();
            continue;
        }
        if (segment == QLatin1String("album") || segment == QLatin1String("song")
            || segment == QLatin1String("playlist") || segment == QLatin1String("music-video")) {
            parts.kind = segment;
        }
    }
    if (!segments.isEmpty())
        parts.id = segments.last();

    parts.trackId = QUrlQuery(parsed).queryItemValue(QStringLiteral("i"));
    return parts;
}

// Turn iTunes lookup results into tracks, in the order the API returned them.
QList<StreamingCatalog::Track> tracksFromItunes(const QJsonArray &results)
{
    QList<StreamingCatalog::Track> tracks;
    for (const QJsonValue &value : results) {
        const QJsonObject item = value.toObject();
        if (item.value("wrapperType").toString() != QLatin1String("track"))
            continue; // the first result of an album lookup is the collection
        StreamingCatalog::Track track;
        track.title = collapseSpaces(item.value("trackName").toString());
        track.artist = collapseSpaces(item.value("artistName").toString());
        if (!track.title.isEmpty())
            tracks.append(track);
    }
    return tracks;
}

// Albums, songs and single album tracks all come from the public iTunes lookup
// API — no key, no scraping, and it answers with clean artist/title fields.
StreamingCatalog::Listing appleFromItunes(const AppleParts &parts, const QString &id,
                                          bool wholeCollection, const Log &log)
{
    StreamingCatalog::Listing listing;
    listing.service = StreamingCatalog::Service::AppleMusic;
    listing.collection = wholeCollection;

    QString lookupUrl = QStringLiteral("https://itunes.apple.com/lookup?id=%1&country=%2")
                            .arg(id, parts.storefront);
    if (wholeCollection)
        lookupUrl += QStringLiteral("&entity=song&limit=200");

    QString error;
    const QJsonDocument doc = httpGetJson(lookupUrl, &error);
    const QJsonArray results = doc.object().value("results").toArray();
    if (results.isEmpty()) {
        listing.error = QStringLiteral("Apple Music returned nothing for that link%1.")
                            .arg(error.isEmpty() ? QString()
                                                 : QStringLiteral(" (%1)").arg(error));
        return listing;
    }

    if (wholeCollection) {
        const QJsonObject collection = results.first().toObject();
        listing.name = collapseSpaces(collection.value("collectionName").toString());
        listing.expectedTotal = collection.value("trackCount").toInt();
    } else {
        listing.expectedTotal = 1;
    }

    listing.tracks = tracksFromItunes(results);
    if (listing.tracks.isEmpty()) {
        listing.error = QStringLiteral("That Apple Music link holds no playable tracks.");
        return listing;
    }

    // The lookup API takes a limit but no offset, so a collection longer than
    // the cap cannot be walked further. Say so instead of downloading a part.
    if (listing.expectedTotal > listing.tracks.size()) {
        listing.truncated = true;
        listing.truncationNote =
            QStringLiteral("Apple Music reports %1 tracks in \"%2\" but only %3 could "
                           "be read.")
                .arg(listing.expectedTotal).arg(listing.name).arg(listing.tracks.size());
    }

    say(log, QStringLiteral("Apple Music: %1 track(s)%2.")
                 .arg(listing.tracks.size())
                 .arg(listing.name.isEmpty() ? QString()
                                             : QStringLiteral(" in \"%1\"").arg(listing.name)));
    return listing;
}

// Playlists (pl.*) are the one Apple Music shape the lookup API doesn't serve.
// The web page embeds a JSON-LD MusicPlaylist listing every track — but only
// its title and its song URL, never the artist. The song ids in those URLs go
// back through the lookup API (batched) to recover the artists.
StreamingCatalog::Listing applePlaylist(const AppleParts &parts, const QString &url,
                                        const Log &log)
{
    StreamingCatalog::Listing listing;
    listing.service = StreamingCatalog::Service::AppleMusic;
    listing.collection = true;

    say(log, QStringLiteral("Reading the Apple Music playlist..."));

    QString error;
    const QByteArray page = httpGet(url, &error);
    if (page.isEmpty()) {
        listing.error = QStringLiteral("Could not reach Apple Music: %1")
                            .arg(error.isEmpty() ? QStringLiteral("empty response") : error);
        return listing;
    }

    static const QRegularExpression ldJson(
        QStringLiteral("<script[^>]*type=\"application/ld\\+json\"[^>]*>(.*?)</script>"),
        QRegularExpression::DotMatchesEverythingOption);
    const QString html = QString::fromUtf8(page);

    QJsonObject playlist;
    QRegularExpressionMatchIterator it = ldJson.globalMatch(html);
    while (it.hasNext()) {
        const QJsonObject candidate =
            QJsonDocument::fromJson(it.next().captured(1).toUtf8()).object();
        if (candidate.contains("track")) {
            playlist = candidate;
            break;
        }
    }
    if (playlist.isEmpty()) {
        listing.error = QStringLiteral(
            "Apple Music did not return the track list for that playlist. Make sure "
            "it is shared publicly.");
        return listing;
    }

    listing.name = collapseSpaces(playlist.value("name").toString());
    // Apple states the playlist's length, which is what makes it possible to
    // prove the list came back whole rather than hope it did.
    listing.expectedTotal = playlist.value("numTracks").toInt();

    // Titles first, in playlist order; the song id (when present) lets the
    // artist be filled in afterwards.
    static const QRegularExpression songId(QStringLiteral("/song/[^/]+/(\\d+)"));
    QStringList ids;
    const QJsonArray trackArray = playlist.value("track").toArray();
    for (const QJsonValue &value : trackArray) {
        const QJsonObject item = value.toObject();
        StreamingCatalog::Track track;
        track.title = collapseSpaces(item.value("name").toString());
        if (track.title.isEmpty())
            continue;
        const QRegularExpressionMatch m = songId.match(item.value("url").toString());
        ids << (m.hasMatch() ? m.captured(1) : QString());
        listing.tracks.append(track);
    }

    if (listing.tracks.isEmpty()) {
        listing.error = QStringLiteral("That Apple Music playlist holds no tracks.");
        return listing;
    }

    // Batch the artist lookup: one request per 50 songs rather than per song.
    say(log, QStringLiteral("Looking up the artists for %1 track(s)...")
                 .arg(listing.tracks.size()));
    QHash<QString, QString> artistById;
    QHash<QString, QString> titleById;
    for (int start = 0; start < ids.size(); start += 50) {
        QStringList chunk;
        for (int i = start; i < qMin(start + 50, ids.size()); ++i) {
            if (!ids.at(i).isEmpty())
                chunk << ids.at(i);
        }
        if (chunk.isEmpty())
            continue;

        const QJsonDocument doc = httpGetJson(
            QStringLiteral("https://itunes.apple.com/lookup?id=%1&country=%2")
                .arg(chunk.join(','), parts.storefront),
            nullptr);
        const QJsonArray results = doc.object().value("results").toArray();
        for (const QJsonValue &value : results) {
            const QJsonObject item = value.toObject();
            const QString trackId = QString::number(item.value("trackId").toVariant().toLongLong());
            const QString artist = collapseSpaces(item.value("artistName").toString());
            if (!artist.isEmpty())
                artistById.insert(trackId, artist);
            const QString title = collapseSpaces(item.value("trackName").toString());
            if (!title.isEmpty())
                titleById.insert(trackId, title);
        }
    }

    int named = 0;
    for (int i = 0; i < listing.tracks.size(); ++i) {
        const QString id = ids.value(i);
        if (id.isEmpty())
            continue;
        const QString artist = artistById.value(id);
        if (!artist.isEmpty()) {
            listing.tracks[i].artist = artist;
            ++named;
        }
        // The store's own title beats the playlist rendering of it.
        const QString title = titleById.value(id);
        if (!title.isEmpty())
            listing.tracks[i].title = title;
    }
    if (named < listing.tracks.size()) {
        say(log, QStringLiteral("Note: %1 track(s) came back without an artist — they "
                                "will be searched by title alone.")
                     .arg(listing.tracks.size() - named));
    }

    if (listing.expectedTotal > listing.tracks.size()) {
        listing.truncated = true;
        listing.truncationNote =
            QStringLiteral("Apple Music reports %1 tracks in \"%2\" but only %3 could "
                           "be read from the playlist page.")
                .arg(listing.expectedTotal).arg(listing.name).arg(listing.tracks.size());
    }

    say(log, QStringLiteral("Apple Music: %1 of %2 track(s) in \"%3\".")
                 .arg(listing.tracks.size()).arg(listing.expectedTotal).arg(listing.name));
    return listing;
}

StreamingCatalog::Listing resolveAppleMusic(const QString &url, const Log &log)
{
    const AppleParts parts = appleParts(url);

    if (parts.kind == QLatin1String("playlist") || parts.id.startsWith(QLatin1String("pl.")))
        return applePlaylist(parts, url, log);

    // "?i=<id>" picks one track out of an album page; a /song/ link is already
    // a single track. Everything else on an album link means the whole album.
    if (!parts.trackId.isEmpty()) {
        say(log, QStringLiteral("Reading the Apple Music track..."));
        return appleFromItunes(parts, parts.trackId, /*wholeCollection=*/false, log);
    }
    if (parts.kind == QLatin1String("song")) {
        say(log, QStringLiteral("Reading the Apple Music song..."));
        return appleFromItunes(parts, parts.id, /*wholeCollection=*/false, log);
    }
    if (parts.kind == QLatin1String("album")) {
        say(log, QStringLiteral("Reading the Apple Music album..."));
        return appleFromItunes(parts, parts.id, /*wholeCollection=*/true, log);
    }

    StreamingCatalog::Listing listing;
    listing.service = StreamingCatalog::Service::AppleMusic;
    listing.error = QStringLiteral(
        "XFB can read Apple Music album, song and playlist links. Artist and radio "
        "station links have no fixed track list to download.");
    return listing;
}

} // namespace

namespace StreamingCatalog {

Service serviceOf(const QString &url)
{
    const QString link = url.trimmed();
    if (link.contains(QLatin1String("open.spotify.com"), Qt::CaseInsensitive)
        || link.startsWith(QLatin1String("spotify:"), Qt::CaseInsensitive))
        return Service::Spotify;
    if (link.contains(QLatin1String("music.apple.com"), Qt::CaseInsensitive))
        return Service::AppleMusic;
    return Service::None;
}

QString serviceName(Service service)
{
    switch (service) {
    case Service::Spotify:    return QStringLiteral("Spotify");
    case Service::AppleMusic: return QStringLiteral("Apple Music");
    case Service::None:       break;
    }
    return QString();
}

bool isCollectionUrl(const QString &url)
{
    switch (serviceOf(url)) {
    case Service::Spotify: {
        QString kind;
        return spotifyParts(url, &kind, nullptr) && kind != QLatin1String("track");
    }
    case Service::AppleMusic: {
        const AppleParts parts = appleParts(url);
        if (parts.kind == QLatin1String("playlist") || parts.id.startsWith(QLatin1String("pl.")))
            return true;
        // An album link with "?i=" is a link to one of its tracks.
        return parts.kind == QLatin1String("album") && parts.trackId.isEmpty();
    }
    case Service::None:
        break;
    }
    return false;
}

Listing resolve(const QString &url, const std::function<void(const QString &)> &log)
{
    const QString link = url.trimmed();
    switch (serviceOf(link)) {
    case Service::Spotify:    return resolveSpotify(link, log);
    case Service::AppleMusic: return resolveAppleMusic(link, log);
    case Service::None:       break;
    }

    Listing listing;
    listing.error = QStringLiteral("That link is not a Spotify or Apple Music link.");
    return listing;
}

QString searchUrlFor(const Track &track)
{
    // Neither service serves the audio, so the track is fetched the same way
    // every other XFB download is: the best YouTube match for artist + title.
    QString terms = collapseSpaces(track.artist + QLatin1Char(' ') + track.title);
    return QStringLiteral("ytsearch1:") + terms;
}

} // namespace StreamingCatalog
