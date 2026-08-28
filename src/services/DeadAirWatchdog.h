#ifndef DEADAIRWATCHDOG_H
#define DEADAIRWATCHDOG_H

#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>

class QTimer;

/**
 * @brief Watches for the one failure a radio station cannot survive quietly.
 *
 * Every other fault announces itself: a missing file logs, a failed upload
 * pops a dialog, a dead stream turns a button red. Silence does none of that.
 * The transport still says "playing", the clock still runs, the meter simply
 * stops moving, and nobody in the building hears anything because the studio
 * monitors are fed from the same dead output. Stations have gone out for
 * hours that way.
 *
 * This object is deliberately dumb about XFB. It is fed three things — the
 * master level, the transport, and a tick — and it emits two things — trip
 * and recover. Everything that knows about playlists, the library or the UI
 * lives in the player; everything that knows about *time* lives here, so the
 * decision can be tested without an audio device, a window or a database.
 *
 * Two failures are detected:
 *
 *  - **Silence.** The master peak stays under a threshold for longer than the
 *    trip delay while there is something to be on air with. "Under" is
 *    measured in dBFS from the peaks the FX engine already emits for the
 *    level meter, which is why the threshold is a dBFS number and not a
 *    fraction: an operator reads a meter in dB.
 *  - **A stalled transport.** The state says playing and the position has
 *    not moved for longer than the trip delay. player.cpp has its own,
 *    faster stall recovery for a single wedged track (6 s, rebuild the sink,
 *    skip on); this one is the longer backstop for when that recovery itself
 *    did not take.
 *
 * Two things it deliberately will not do:
 *
 *  - It will not trip on a level it cannot measure. `FxPlayer::levels` is
 *    emitted only while the FX engine is the active path; ordinary
 *    passthrough playback produces no samples at all. A watchdog that read
 *    "no samples" as "no audio" would fire on every healthy station that has
 *    no effects enabled, so silence detection is armed only while levels are
 *    actually arriving, and the stall detector covers the rest.
 *  - It will not trip twice in a row over the same outage. After a trip it
 *    goes into a cooldown, and it will not arm again until audio has been
 *    healthy for the recovery window. That is what stops "rescue, fail,
 *    rescue, fail" from turning into a loop that fills the log and the
 *    as-run report with hundreds of rows.
 */
class DeadAirWatchdog : public QObject
{
    Q_OBJECT

public:
    /** Why the watchdog tripped. */
    enum class Reason {
        None,
        Silence,    ///< master level under the threshold for the trip delay
        Stall,      ///< position frozen while the transport claims playing
        Stopped,    ///< nothing playing at all, with a running order loaded
    };
    Q_ENUM(Reason)

    /** What the station is doing right now, as the heartbeat reports it. */
    enum class AirState {
        Off,        ///< XFB is not trying to be on air
        Playing,    ///< audio is moving
        Silent,     ///< transport playing, meter dead
        Stalled,    ///< transport playing, position frozen
        Fallback,   ///< the watchdog is holding the station up
    };
    Q_ENUM(AirState)

    /** Everything the operator can set, and what it means. */
    struct Config
    {
        bool    enabled          = false;
        /** Peak below this counts as silence. -50 dBFS is below the noise
         *  floor of any real programme and above a truly dead output. */
        double  silenceDb        = -50.0;
        /** How long that has to last before the station is rescued. */
        int     tripSeconds      = 20;
        /** How long healthy audio has to last before the incident is over. */
        int     recoverSeconds   = 10;
        /** Minimum quiet time between two trips. */
        int     rearmSeconds     = 120;
        /** A saved playlist XML to run, or empty. */
        QString fallbackPlaylist;
        /** A folder of evergreen tracks to run, or empty. Used when no
         *  playlist is set, or when the playlist turns out to be unusable. */
        QString fallbackFolder;
        /** Play the folder in a random order rather than by name. */
        bool    shuffleFolder    = true;
        /** How many fallback tracks to queue on a trip. */
        int     fallbackTracks   = 12;
        /** Put an alert on screen. */
        bool    notifyScreen     = true;
        /** Publish the incident where a paired phone can poll it. */
        bool    notifyPhone      = true;
        /** Count "a running order is loaded but nothing is playing" as dead
         *  air. Off means only silence and stalls trip. */
        bool    watchStopped     = true;
    };

    explicit DeadAirWatchdog(QObject *parent = nullptr);

    static Config loadConfig();
    static void   saveConfig(const Config &config);
    /** The settings file every other XFB setting lives in. */
    static QString configPath();

    Config config() const { return m_config; }
    /** Applies and persists. Turning it off ends any incident cleanly. */
    void setConfig(const Config &config);

    /** True while the fallback is holding the station up. (Not named
     *  tripped(): that is the signal.) */
    bool isTripped() const { return m_tripped; }
    Reason lastReason() const { return m_lastReason; }
    AirState airState() const;
    /** When the current incident started; invalid when there is none. */
    QDateTime incidentStarted() const { return m_incidentStarted; }
    /** How long the watchdog has been running, in seconds. */
    qint64 uptimeSeconds() const;
    /** The last level seen, in dBFS. -120 when nothing has arrived. */
    double lastLevelDb() const { return m_levelDb; }
    /** A one-line description of the last incident, for status displays. */
    QString lastIncident() const { return m_lastIncident; }

    // --- what the player feeds in -------------------------------------------

    /** Master peaks, 0..1, straight off FxPlayer::levels. */
    void noteLevels(float left, float right);

    /**
     * The transport, sampled once per tick by the caller.
     *
     * @param playing        the transport claims to be playing
     * @param positionMs     where it claims to be
     * @param haveProgramme  there is something to be on air with (a running
     *                       order loaded, or a track on air)
     * @param levelsLive     the FX engine is the active path, so silence can
     *                       actually be measured
     */
    void noteTransport(bool playing, qint64 positionMs, bool haveProgramme,
                       bool levelsLive);

    /** The operator pressed Stop. Silence after that is intentional. */
    void noteOperatorStop();
    /** Playback started; whatever the operator stopped, they have restarted. */
    void noteOperatorPlay();

    /**
     * One evaluation. Public and taking its own clock so the state machine
     * can be driven from a test with synthetic times; in the running app a
     * 1 Hz timer calls it with the real elapsed clock.
     */
    void tick(qint64 nowMs);

    /** Drives tick() from a QTimer. On by default; a test turns it off. */
    void setAutoTick(bool on);

    /**
     * The player calls this once it knows whether the rescue worked. A
     * fallback that could not be started is worse than one that could, and
     * the operator has to be told a different thing.
     */
    void noteFallbackStarted(bool ok, const QString &what);

    /** Ends the incident by hand ("I have fixed it"), without waiting. */
    void clearIncident();

    // --- fallback material ---------------------------------------------------

    /**
     * The tracks to run, resolved from the configuration: the playlist XML
     * first, the folder second. Static because the resolution is pure and
     * both the player and the config dialog (which previews the count) want
     * it. Returns absolute paths that exist.
     */
    static QStringList resolveFallbackTracks(const Config &config);

    /** Reads an XFB playlist XML and returns the file paths in it. */
    static QStringList tracksFromPlaylistFile(const QString &path);
    /** Audio files directly inside a folder, sorted by name. */
    static QStringList tracksFromFolder(const QString &folder);
    /** The extensions XFB will consider evergreen material. */
    static QStringList audioSuffixes();

    /** dBFS for a linear 0..1 peak, floored at -120. */
    static double linearToDb(double linear);

signals:
    /**
     * The station is dark and something has to be done about it. The player
     * answers this by queueing the fallback material and starting it.
     */
    void tripped(DeadAirWatchdog::Reason reason, const QString &detail);

    /** The incident is over — audio came back, or the fallback took hold. */
    void recovered(const QString &detail);

    /** An alert the operator should see. @p critical picks the loud one. */
    void alert(const QString &title, const QString &message, bool critical);

    /** Something worth a line in xfb.log and the status bar. */
    void logMessage(const QString &message);

    /** Anything a status display would want to redraw for. */
    void stateChanged();

private:
    void trip(Reason reason, const QString &detail);
    void endIncident(const QString &detail, bool recovered);
    void openIncidentRow(Reason reason, const QString &detail);
    void closeIncidentRow(qint64 playedMs);
    static QString reasonWord(Reason reason);

    Config m_config;

    QTimer  *m_timer = nullptr;
    QElapsedTimer m_clock;      ///< monotonic, started with the object

    // what the player last told us
    double  m_levelDb        = -120.0;
    /// Whether a level sample arrived since the last tick, and how many ticks
    /// have gone by without one. Counted in ticks rather than stamped with a
    /// wall clock so the whole decision runs off the tick's own time — which
    /// is what lets a test drive months of station time in a millisecond.
    bool    m_levelSeen      = false;
    bool    m_levelEverSeen  = false;
    int     m_ticksWithoutLevel = 1000;
    bool    m_playing        = false;
    qint64  m_positionMs     = -1;
    bool    m_haveProgramme  = false;
    bool    m_levelsLive     = false;

    // the running measurement
    qint64  m_silentSinceMs  = -1;   ///< first tick of the current silence
    qint64  m_stalledSinceMs = -1;
    qint64  m_healthySinceMs = -1;
    qint64  m_lastPositionMs = -1;
    qint64  m_lastTripEndMs  = -1;   ///< cooldown anchor
    bool    m_operatorStopped = false;
    bool    m_everOnAir      = false;

    // the incident
    bool      m_tripped        = false;
    Reason    m_lastReason     = Reason::None;
    QDateTime m_incidentStarted;
    QString   m_lastIncident;
    qint64    m_incidentAirHandle = 0;
    qint64    m_incidentStartMs   = -1;
};

#endif // DEADAIRWATCHDOG_H
