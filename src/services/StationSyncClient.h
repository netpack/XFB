#ifndef STATIONSYNCCLIENT_H
#define STATIONSYNCCLIENT_H

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

/**
 * @brief Keeps this XFB a ready-to-run copy of another one on the network.
 *
 * A radio station that loses its studio machine loses its air. The remedy this
 * implements is an ordinary second machine running XFB in the corner, holding
 * the same music, the same jingles, the same ads, the same programs, the same
 * schedule and the same saved playlists as the one on air — so that going back
 * on air is a matter of launching it, not of restoring a backup.
 *
 * The copy is pulled, never pushed. This side opens every connection, which
 * means the backup can sit behind a firewall, be switched off for a fortnight
 * or be moved to another building, and the station it mirrors neither knows
 * nor cares. It also means the station on air never spends its own time on the
 * backup's schedule.
 *
 * Pairing is the same six-digit exchange the phone companion uses, except that
 * the operator opens the window from the Station Backup dialog, which is what
 * makes the resulting token a station token — see MobileSyncServer::PeerRole.
 * The transport is plain HTTP on the local network: the token keeps strangers
 * out, it does not encrypt what travels, so this belongs on a network the
 * station controls.
 *
 * Nothing is destroyed on the way in. Media files are written beside whatever
 * this machine already has (an interrupted download resumes rather than
 * starting again), and a catalogue row is only written once its file has
 * actually arrived, so a half-finished sync leaves a smaller working station
 * rather than a full one full of dead entries.
 */
class StationSyncClient : public QObject
{
    Q_OBJECT

public:
    /** What the sync is busy with, for the operator's benefit. */
    enum class Stage {
        Idle,
        Connecting,
        Files,
        Playlists,
        Catalogue,
        Settings,
        Done,
    };
    Q_ENUM(Stage)

    explicit StationSyncClient(QObject *parent = nullptr);
    ~StationSyncClient() override;

    // --- the station this one mirrors ---------------------------------------

    QString peerHost() const { return m_host; }
    quint16 peerPort() const { return m_port; }
    void setPeer(const QString &host, quint16 port);

    /** True once a station has handed this machine a token. */
    bool paired() const { return !m_token.isEmpty(); }
    /** Name the station gave itself when pairing, for display. */
    QString peerName() const { return m_peerName; }
    /** Drops the token and the address; the media already fetched is kept. */
    void forgetPeer();

    // --- when it happens by itself ------------------------------------------

    /** 0 means "only when the operator asks". */
    int autoSyncMinutes() const { return m_autoSyncMinutes; }
    void setAutoSyncMinutes(int minutes);
    bool syncOnStart() const { return m_syncOnStart; }
    void setSyncOnStart(bool on);

    QDateTime lastSync() const { return m_lastSync; }
    QString lastResult() const { return m_lastResult; }

    // --- watching the studio -------------------------------------------------

    /**
     * A mirror of last week's music is not a backup if nobody notices that
     * the studio has gone quiet. This polls the station's own heartbeat and
     * raises the alarm on the machine standing by.
     *
     * It never touches this machine's transport. Deciding to go on air is a
     * human decision with a human's judgement behind it — see
     * player::monitorTakeOver() for the manual flow this stops short of.
     */
    int monitorSeconds() const { return m_monitorSeconds; }
    /** 0 turns the heartbeat off entirely. */
    void setMonitorSeconds(int seconds);

    /** How long the studio may be dark or unreachable before the alarm. */
    int darkAfterSeconds() const { return m_darkAfterSeconds; }
    void setDarkAfterSeconds(int seconds);

    /** True while the alarm is standing. */
    bool studioIsDark() const { return m_studioDark; }
    /** The last word the studio used about itself, or why it could not. */
    QString studioState() const { return m_studioState; }
    /** When a heartbeat last said the studio was making sound. */
    QDateTime lastGoodHeartbeat() const { return m_lastGoodHeartbeat; }

    bool busy() const { return m_busy; }
    Stage stage() const { return m_stage; }

    /** Where files of a given category land on this machine. */
    QString localRoot(const QString &category) const;
    /** Overrides the folder used for a category ("musics", "jingles", ...). */
    void setLocalRoot(const QString &category, const QString &path);

    /** The categories this client mirrors, in manifest order. */
    static QStringList categories();

public slots:
    /** Asks @p host for a token using the six-digit code it is showing. */
    void pairWith(const QString &host, quint16 port, const QString &code);
    /** Pulls everything that has changed since last time. */
    void sync();
    /** Stops after the file being fetched right now. */
    void cancel();

signals:
    void pairingSucceeded(const QString &stationName);
    void pairingFailed(const QString &reason);

    void busyChanged(bool busy);
    void stageChanged(StationSyncClient::Stage stage, const QString &description);
    /** @p percent is -1 while the total is still unknown. */
    void progress(int percent, const QString &message);
    void finished(const QString &summary);
    void failed(const QString &reason);

    /** Every answered heartbeat, for a status display. */
    void heartbeat(const QJsonObject &state);
    /**
     * The studio has been dark or unreachable for longer than the configured
     * period. Loud, actionable, and on this machine only — nothing switches
     * itself on air on the strength of it.
     */
    void studioWentDark(const QString &reason);
    /** ...and it came back. */
    void studioCameBack(const QString &detail);

private:
    /** One file this machine still has to fetch. */
    struct FileJob {
        QString id;
        QString category;
        QString relative;
        QString target;     ///< absolute path on this machine
        qint64  bytes = 0;
        qint64  modified = 0;
    };

    void load();
    void save();

    QNetworkReply *get(const QString &path, qint64 resumeFrom = -1);

    void onManifest(const QByteArray &body);
    void planFiles(const QJsonObject &manifest);
    void startNextFile();

    void startNextPlaylist();

    void applyCatalogue();
    void applySettings();
    void completeSync();
    void abortSync(const QString &reason);

    void pollHeartbeat();
    void applyHeartbeat(bool reachable, const QJsonObject &state,
                        const QString &error);
    void restartMonitor();

    void setBusy(bool busy);
    void setStage(Stage stage, const QString &description);
    void reportProgress(const QString &message = QString());

    QNetworkAccessManager *m_net = nullptr;
    QTimer *m_autoTimer = nullptr;
    QTimer *m_monitorTimer = nullptr;
    QNetworkReply *m_heartbeatReply = nullptr;

    QString m_host;
    quint16 m_port = 0;
    QString m_token;
    QString m_peerName;
    int m_autoSyncMinutes = 0;
    bool m_syncOnStart = false;

    // watching the studio
    int  m_monitorSeconds = 0;      ///< 0: not watching
    int  m_darkAfterSeconds = 90;
    bool m_studioDark = false;
    QString m_studioState;
    QDateTime m_lastGoodHeartbeat;

    QDateTime m_lastSync;
    QString m_lastResult;
    QHash<QString, QString> m_roots;   ///< category -> folder override

    bool m_busy = false;
    bool m_cancelled = false;
    Stage m_stage = Stage::Idle;

    QJsonObject m_manifest;
    QQueue<FileJob> m_pending;
    QStringList m_pendingPlaylists;
    QNetworkReply *m_currentReply = nullptr;

    qint64 m_bytesTotal = 0;
    qint64 m_bytesDone = 0;
    int m_filesFetched = 0;
    int m_filesFailed = 0;
    int m_filesAlreadyHere = 0;
    int m_playlistsFetched = 0;
    int m_rowsWritten = 0;
    int m_rowsPending = 0;
};

#endif // STATIONSYNCCLIENT_H
