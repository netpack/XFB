#ifndef LIBRARYWATCHER_H
#define LIBRARYWATCHER_H

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QFileSystemWatcher;
class QTimer;

/**
 * @brief Folders XFB keeps an eye on, so material that lands in one is in the
 *        database without anybody opening a window.
 *
 * The importer in add_full_dir is a thing an operator *does*: point it at a
 * folder, press Add, read the summary. That is the right shape for a library
 * being taken on, and the wrong shape for the way material actually arrives at
 * a station — a jingle package dropped on the studio share by the production
 * desk, an advert pushed overnight by the agency, this week's programme copied
 * off a stick. Somebody has to remember to import those, and the day nobody
 * does is the day the advert does not go out.
 *
 * A watched folder says where new files belong once and for all: this folder
 * is jingles, that one is adverts, the big one is music. Anything that turns
 * up in one and is not in the library yet is filed under it.
 *
 * Two things it deliberately does not do:
 *
 *  - **It does not import a file that is still arriving.** A copy over the
 *    network appears in the folder immediately and grows for the next minute;
 *    a file imported at that moment gets its duration read from a fragment and
 *    plays as a fragment for ever. A file therefore has to have stopped
 *    changing for `quietSeconds`, measured across two passes, before it counts
 *    as having landed.
 *  - **It does not remove anything.** A file that disappears from a watched
 *    folder is left in the database, where the existing "check the records"
 *    pass deals with it. Deleting library rows from a background timer, on the
 *    evidence of a share that might simply be unmounted, is not a thing a
 *    station can afford.
 *
 * Detection is a poll, with the filesystem watcher as an accelerator rather
 * than the mechanism: QFileSystemWatcher reports the folders it was given and
 * nothing below them, and it reports nothing at all on most network shares —
 * which is exactly where a station's drop folder lives. The timer is what
 * makes it work; the watcher only makes it feel immediate when the folder is
 * local.
 */
class LibraryWatcher : public QObject
{
    Q_OBJECT

public:
    /** Which of the four libraries a watched folder feeds. */
    enum class Destination {
        Music,      ///< musics — needs an artist, a title and a genre
        Jingles,    ///< jingles
        Publicity,  ///< pub (adverts)
        Programs,   ///< programs
    };
    Q_ENUM(Destination)

    /** One watched folder and what lands in it. */
    struct Folder
    {
        QString     path;
        Destination destination  = Destination::Music;
        /** Descend into subfolders. */
        bool        recursive    = true;
        /** Music only: the first folder below this one names the genre, the
         *  way the folder importer does it. */
        bool        folderGenres = true;
        /** Music only: the genre for anything the rule above cannot name. */
        QString     genre;
        /** Watch this one. Off keeps the row without acting on it. */
        bool        enabled      = true;
    };

    struct Config
    {
        bool  enabled      = false;
        /** How often every watched folder is walked. */
        int   pollSeconds  = 60;
        /** How long a file has to have stopped changing before it is taken. */
        int   quietSeconds = 15;
        /** At most this many files are imported in one pass, so the first
         *  pass over a large library cannot hold the UI for minutes. What is
         *  left is taken on the next pass. */
        int   batchLimit   = 100;
        QList<Folder> folders;
    };

    explicit LibraryWatcher(QObject *parent = nullptr);

    static QString configPath();
    static Config  loadConfig();
    static void    saveConfig(const Config &config);

    Config config() const { return m_config; }
    /** Applies and persists. Turning it on runs a pass straight away. */
    void setConfig(const Config &config);

    /** Walk every enabled folder now and import what has landed.
     *  @return how many rows were added. */
    int scanNow();

    /**
     * What one folder would import if everything in it had settled: files
     * that are audio, are under the folder, and are not in its destination
     * table yet. Used by the settings window to say what is waiting, without
     * importing anything.
     */
    static int pendingCount(const Folder &folder);

    QDateTime lastScan() const { return m_lastScan; }
    /** A sentence about the last pass, for the settings window. */
    QString lastSummary() const { return m_lastSummary; }

    // The destination as a translated label, and as the string that goes in
    // xfb.conf (which must never be translated — a settings file written in
    // Portuguese has to still read on an English machine).
    static QString     destinationLabel(Destination destination);
    static QString     destinationKey(Destination destination);
    static Destination destinationFromKey(const QString &key);
    /** The table each destination writes to. */
    static QString     destinationTable(Destination destination);

signals:
    /**
     * Rows were added. The player refreshes the library views and says so;
     * nothing else in XFB needs to know.
     * @param count  how many rows
     * @param what   a sentence naming what went where
     */
    void imported(int count, const QString &what);

    /** For the log and the status line — never a dialog. A watched folder
     *  that has gone missing must not stop the station with a modal box. */
    void logMessage(const QString &message);

    /** Configuration changed, so an open settings window can catch up. */
    void stateChanged();

private slots:
    void onDirectoryChanged(const QString &path);
    void onPollTimeout();

private:
    /** Everything the watcher knows about a file it has seen but not taken. */
    struct Seen
    {
        qint64 size    = -1;
        qint64 modTime = -1;   ///< ms since epoch
        qint64 firstSeenMs = 0;
    };

    void rearmWatcher();
    void restartTimer();
    /** @return rows added; appends a "12 to music" style phrase to @p report. */
    int  scanFolder(const Folder &folder, int budget, QStringList *report);
    /** True when the file has stopped changing for long enough to be taken. */
    bool hasSettled(const QString &path);
    bool alreadyInLibrary(const QString &path, Destination destination) const;
    bool importFile(const QString &path, const Folder &folder,
                    const QString &genre, QString *error);
    /** The genre a music file inherits from where it sits, reconciled against
     *  genres1 so a "rock" folder files under an existing "Rock". */
    QString genreForFile(const QString &path, const Folder &folder);

    Config m_config;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_pollTimer = nullptr;
    /** Coalesces the burst of directoryChanged a single copy produces. */
    QTimer *m_settleTimer = nullptr;

    QHash<QString, Seen> m_seen;
    /** Files whose import was refused, so the log does not repeat every pass. */
    QHash<QString, QString> m_refused;
    /** Genre names from genres1, keyed lower-case, refreshed per pass. */
    QHash<QString, QString> m_knownGenres;

    QDateTime m_lastScan;
    QString   m_lastSummary;
    bool      m_scanning = false;
};

#endif // LIBRARYWATCHER_H
