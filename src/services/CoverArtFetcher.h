#ifndef COVERARTFETCHER_H
#define COVERARTFETCHER_H

#include <QImage>
#include <QString>
#include <QVector>

class QSqlDatabase;

/**
 * Puts cover art on the library's older downloads.
 *
 * XFB has embedded a cover into every download since the tagging pass learned
 * to carry one (an Opus file keeps its picture as a METADATA_BLOCK_PICTURE
 * comment, and the pass used to throw it away along with the tags). Anything
 * downloaded before that has no picture and never will on its own — nothing
 * else on the desk knows what the cover looked like, and the phone's
 * notification, the lock screen and the deck all read the cover out of the
 * file. This is the way back for those.
 *
 * The picture is found the way the download would have found it: the link the
 * track came from when the library still has it, and otherwise a yt-dlp search
 * for the artist and title, which for a library of uploads with distinctive
 * titles usually lands back on the original. "Usually" is why nothing is
 * written until the operator has seen what was found — a search can match the
 * wrong thing, and a wrong cover embedded in 300 files is worse than none.
 *
 * Every step is separate and cancellable: finding the tracks, finding pictures
 * for them, and writing those pictures in. Only the last one touches a file.
 */
class CoverArtFetcher
{
public:
    /** One track that has no cover, and whatever has been found for it. */
    struct Candidate
    {
        QString filePath;
        QString artist;
        QString song;
        /** musics.source_url, empty for anything not downloaded by XFB. */
        QString sourceUrl;

        /** The picture found for it, on disk in the cache. Empty until found. */
        QString imagePath;
        QImage preview;
        /** How it was found, in words, for the operator to judge it by. */
        QString foundVia;
        /** Why nothing was found, or why writing it failed. */
        QString problem;

        bool found() const { return !imagePath.isEmpty(); }
    };

    /** Whether this file already carries a picture. */
    static bool hasCover(const QString &filePath);

    /**
     * Every track in the library whose file exists and carries no picture,
     * newest rows first. Cancel by returning false from *keepGoing*, which is
     * also where progress is reported: this reads every file on the desk.
     */
    static QVector<Candidate> findTracksWithoutCover(
        QSqlDatabase &db,
        const std::function<bool(int done, int total, const QString &what)> &keepGoing);

    /**
     * Finds a picture for one candidate and leaves it in the cache. Fills in
     * imagePath/preview/foundVia on success, problem on failure. Never writes
     * to the audio file.
     */
    static void findCoverFor(Candidate &candidate);

    /**
     * Writes the found picture into the audio file, preserving the tags it
     * already has. Returns false and sets problem on failure, leaving the
     * original untouched.
     */
    static bool embedCover(Candidate &candidate);

    /** Where fetched pictures are kept between finding and writing. */
    static QString cacheDirectory();

private:
    static QString ffmpegPath();
    static QString ffprobePath();
    static QString ytDlpPath();
    static QByteArray pictureBlockFor(const QString &imagePath);
    static QString fetchThumbnail(const QString &target, const QString &basePath);
};

#endif // COVERARTFETCHER_H
