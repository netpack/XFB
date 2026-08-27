#ifndef PRODUCTIONSYNCCLIENT_H
#define PRODUCTIONSYNCCLIENT_H

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

/**
 * @brief Makes this XFB a production computer for a station that is on air
 *        somewhere else on the network.
 *
 * A station prepares far more than it plays. Next week's music has to be
 * tagged, this month's ads cut and scheduled, a jingle re-recorded, the hour
 * grid rearranged — and none of that wants doing on the machine that is
 * broadcasting, where a mis-click is heard by everybody listening. The usual
 * answer is a second computer in the office; the awkward part has always been
 * getting the work from that computer onto the one on air.
 *
 * This is both halves of that:
 *
 *  - **Down.** The station's catalogue, media, jingles, ads, programs, schedule
 *    and saved playlists are copied onto this machine, so the operator here is
 *    working with what is actually on air rather than a guess at it, and can
 *    audition any of it locally without touching the broadcast.
 *  - **Up.** Whatever is added or changed here — a new track, a new jingle, a
 *    new ad, a rearranged hour — is published back to the station, where Auto
 *    Mode picks it up on its very next choice, because every pick is a fresh
 *    query against the station's own database.
 *
 * It is deliberately *not* a mirror. StationSyncClient replaces what it finds,
 * which is exactly right for a backup and exactly wrong here: a production
 * machine's whole purpose is to hold work the station does not have yet, and a
 * mirror would erase it on the next pull. So the copy down merges — rows the
 * station sent are written, rows only this machine has are left alone — and
 * the record of what was last agreed with the station is what tells the two
 * apart when it is time to publish.
 *
 * Where the two ends disagree about the same entry, the station wins: it is
 * the one on air, and the operator here can see the change and redo it. The
 * exception is an entry this machine has changed and the station has not,
 * which is the ordinary case of unpublished work and is kept.
 *
 * Both directions run over the pairing and the token the phone companion and
 * the backup station already use — one port, one pairing flow — with a role of
 * its own, because this is the only role allowed to write to the station.
 */
class ProductionSyncClient : public QObject
{
    Q_OBJECT

public:
    /** What the sync is busy with, for the operator's benefit. */
    enum class Stage {
        Idle,
        Connecting,
        Files,          ///< fetching media from the station
        Playlists,
        Catalogue,      ///< merging the station's rows into this database
        Settings,
        Scanning,       ///< working out what this machine has that the station has not
        Uploading,      ///< sending those files up
        Publishing,     ///< sending the rows that point at them
        Done,
    };
    Q_ENUM(Stage)

    explicit ProductionSyncClient(QObject *parent = nullptr);
    ~ProductionSyncClient() override;

    // --- the station this machine produces for ------------------------------

    QString peerHost() const { return m_host; }
    quint16 peerPort() const { return m_port; }
    void setPeer(const QString &host, quint16 port);

    /** True once the station has handed this machine a production token. */
    bool paired() const { return !m_token.isEmpty(); }
    QString peerName() const { return m_peerName; }
    /** Drops the token and the address. Nothing already copied is removed. */
    void forgetPeer();

    // --- when it happens by itself ------------------------------------------

    /** 0 means "only when the operator asks". */
    int autoSyncMinutes() const { return m_autoSyncMinutes; }
    void setAutoSyncMinutes(int minutes);
    bool syncOnStart() const { return m_syncOnStart; }
    void setSyncOnStart(bool on);
    /** Whether an automatic run publishes as well as fetches. */
    bool publishAutomatically() const { return m_publishAutomatically; }
    void setPublishAutomatically(bool on);

    QDateTime lastSync() const { return m_lastSync; }
    QString lastResult() const { return m_lastResult; }

    bool busy() const { return m_busy; }
    Stage stage() const { return m_stage; }

    /** Where files of a given category live on this machine. */
    QString localRoot(const QString &category) const;
    void setLocalRoot(const QString &category, const QString &path);

    // --- what is waiting to go up -------------------------------------------

    /**
     * A line per kind of pending change ("14 tracks", "the hour schedule"),
     * as last worked out by refreshPendingChanges() or by a sync.
     */
    QStringList pendingChanges() const { return m_pendingSummary; }
    /** Recounts what this machine holds that the station has not been told of. */
    void refreshPendingChanges();

public slots:
    /** Asks @p host for a production token using the six digits it is showing. */
    void pairWith(const QString &host, quint16 port, const QString &code);
    /** Copies down whatever has changed on the station. */
    void pull();
    /** Sends up whatever has been prepared here. */
    void publish();
    /** Both, in the order that matters: down first, then up. */
    void sync();
    /** Stops after the file being transferred right now. */
    void cancel();

signals:
    void pairingSucceeded(const QString &stationName);
    void pairingFailed(const QString &reason);

    void busyChanged(bool busy);
    void stageChanged(ProductionSyncClient::Stage stage, const QString &description);
    /** @p percent is -1 while the total is still unknown. */
    void progress(int percent, const QString &message);
    void finished(const QString &summary);
    void failed(const QString &reason);
    void pendingChangesChanged();

private:
    /** One file this machine still has to fetch from the station. */
    struct FileJob {
        QString id;
        QString category;
        QString relative;
        QString target;
        qint64  bytes = 0;
    };

    /** One entry this machine has that the station has not been told about. */
    struct PendingRow {
        QString table;      ///< which media table it belongs to
        QString relative;   ///< where its file goes under the station's root
        QString path;       ///< where its file is here
        qint64  bytes = 0;
        QJsonObject row;    ///< the columns, minus the path
        QString hash;
    };

    void load();
    void save();

    void loadBaseline();
    void saveBaseline();

    QNetworkReply *get(const QString &path, qint64 resumeFrom = -1);
    QNetworkReply *post(const QString &path, const QByteArray &body,
                        const QByteArray &contentType);

    // fetching
    void onManifest(const QByteArray &body);
    void planFiles(const QJsonObject &manifest);
    void startNextFile();
    void startNextPlaylist();
    void mergeCatalogue();
    void applySettings();
    void afterPull();

    // publishing
    void startPublish();
    void scanForChanges();
    void startNextUpload();
    void uploadChunk();
    void sendNextRowBatch();
    void startNextPlaylistUpload();
    void finishPublish();

    void completeRun();
    void abortRun(const QString &reason);

    void setBusy(bool busy);
    void setStage(Stage stage, const QString &description);
    void reportProgress(const QString &message = QString());

    /** The position @p path would take under the station's own category root. */
    QString relativeFor(const QString &path, const QString &category) const;
    /**
     * A stable fingerprint of a row's columns, used to spot a change.
     *
     * @p ignoreColumn is the table's own key column where it has one. Two XFBs
     * number their rows independently, and a row that had to be renumbered on
     * arrival is not a row that changed — leaving the number out is what stops
     * a renumbering being published back and forth for ever.
     */
    static QString hashRow(const QJsonObject &row, const QString &ignoreColumn = QString());
    /** The same for a whole table, order and all thrown away. */
    static QString hashRows(const QVector<QJsonObject> &rows,
                            const QString &ignoreColumn = QString());
    static QString hashText(const QString &text);

    QNetworkAccessManager *m_net = nullptr;
    QTimer *m_autoTimer = nullptr;

    QString m_host;
    quint16 m_port = 0;
    QString m_token;
    QString m_peerName;
    int m_autoSyncMinutes = 0;
    bool m_syncOnStart = false;
    bool m_publishAutomatically = true;
    QDateTime m_lastSync;
    QString m_lastResult;
    QHash<QString, QString> m_roots;

    bool m_busy = false;
    bool m_cancelled = false;
    bool m_publishAfterPull = false;
    Stage m_stage = Stage::Idle;

    /**
     * What this machine and the station last agreed on: per media table, the
     * fingerprint of every row keyed by where its file sits; per configuration
     * table, one fingerprint for the lot; per playlist, one for the file. A
     * row that differs from this is either new here or newly changed here, and
     * that is the whole of how publishing knows what to send.
     */
    QHash<QString, QHash<QString, QString>> m_baselineRows;
    QHash<QString, QString> m_baselineTables;
    QHash<QString, QString> m_baselinePlaylists;

    QJsonObject m_manifest;
    QQueue<FileJob> m_pending;
    QStringList m_pendingPlaylists;
    QNetworkReply *m_currentReply = nullptr;

    // publishing state
    QQueue<PendingRow> m_uploads;       ///< files still to go up
    QVector<PendingRow> m_readyRows;    ///< rows whose file is up there now
    QHash<QString, QString> m_changedTables;   ///< config table -> its new hash
    QStringList m_changedPlaylists;
    PendingRow m_currentUpload;
    qint64 m_uploadOffset = 0;
    /** Bounded, so a station that keeps disagreeing about the offset ends it. */
    int m_uploadConflicts = 0;
    /** Rows whose files are up there, grouped into requests. */
    QQueue<QPair<QString, QVector<PendingRow>>> m_rowBatches;
    QStringList m_pendingConfigTables;
    /** Per media table, the entries taken out here that the station still has. */
    QHash<QString, QStringList> m_removals;
    QStringList m_pendingSummary;

    qint64 m_bytesTotal = 0;
    qint64 m_bytesDone = 0;
    int m_filesFetched = 0;
    int m_filesFailed = 0;
    int m_filesAlreadyHere = 0;
    int m_playlistsFetched = 0;
    int m_rowsWritten = 0;
    int m_rowsKept = 0;
    int m_filesPublished = 0;
    int m_rowsPublished = 0;
    int m_rowsRemoved = 0;
    int m_filesDeletedThere = 0;
    int m_publishFailures = 0;
    int m_playlistsPublished = 0;
};

#endif // PRODUCTIONSYNCCLIENT_H
