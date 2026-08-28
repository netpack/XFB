#include "DeadAirWatchdog.h"
#include "../audioformats.h"

#include "AirLog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QXmlStreamReader>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

const char *const kGroup = "Watchdog";

/** One evaluation a second: fine enough for a delay measured in seconds,
 *  cheap enough to leave running for months. */
constexpr int kTickMs = 1000;

/** Ticks without a level sample after which the meter feed counts as gone
 *  (the FX engine is not the active path) rather than as quiet audio. */
constexpr int kLevelStaleTicks = 3;

} // namespace

DeadAirWatchdog::DeadAirWatchdog(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
    m_config = loadConfig();

    m_timer = new QTimer(this);
    m_timer->setInterval(kTickMs);
    connect(m_timer, &QTimer::timeout, this,
            [this]() { tick(m_clock.elapsed()); });
    m_timer->start();
}

// ------------------------------------------------------------------ settings

QString DeadAirWatchdog::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

DeadAirWatchdog::Config DeadAirWatchdog::loadConfig()
{
    Config config;
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    config.enabled = settings.value(QStringLiteral("Enabled"), false).toBool();
    config.silenceDb = settings.value(QStringLiteral("SilenceDb"), -50.0).toDouble();
    config.tripSeconds = settings.value(QStringLiteral("TripSeconds"), 20).toInt();
    config.recoverSeconds = settings.value(QStringLiteral("RecoverSeconds"), 10).toInt();
    config.rearmSeconds = settings.value(QStringLiteral("RearmSeconds"), 120).toInt();
    config.fallbackPlaylist = settings.value(QStringLiteral("FallbackPlaylist")).toString();
    config.fallbackFolder = settings.value(QStringLiteral("FallbackFolder")).toString();
    config.shuffleFolder = settings.value(QStringLiteral("ShuffleFolder"), true).toBool();
    config.fallbackTracks = settings.value(QStringLiteral("FallbackTracks"), 12).toInt();
    config.notifyScreen = settings.value(QStringLiteral("NotifyScreen"), true).toBool();
    config.notifyPhone = settings.value(QStringLiteral("NotifyPhone"), true).toBool();
    config.watchStopped = settings.value(QStringLiteral("WatchStopped"), true).toBool();
    settings.endGroup();

    // A nonsense delay is worse than a default one: a trip delay of zero
    // would rescue the station between two drum beats.
    config.tripSeconds = qBound(3, config.tripSeconds, 600);
    config.recoverSeconds = qBound(2, config.recoverSeconds, 600);
    config.rearmSeconds = qBound(10, config.rearmSeconds, 3600);
    config.fallbackTracks = qBound(1, config.fallbackTracks, 200);
    config.silenceDb = qBound(-90.0, config.silenceDb, -10.0);
    return config;
}

void DeadAirWatchdog::saveConfig(const Config &config)
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    settings.setValue(QStringLiteral("Enabled"), config.enabled);
    settings.setValue(QStringLiteral("SilenceDb"), config.silenceDb);
    settings.setValue(QStringLiteral("TripSeconds"), config.tripSeconds);
    settings.setValue(QStringLiteral("RecoverSeconds"), config.recoverSeconds);
    settings.setValue(QStringLiteral("RearmSeconds"), config.rearmSeconds);
    settings.setValue(QStringLiteral("FallbackPlaylist"), config.fallbackPlaylist);
    settings.setValue(QStringLiteral("FallbackFolder"), config.fallbackFolder);
    settings.setValue(QStringLiteral("ShuffleFolder"), config.shuffleFolder);
    settings.setValue(QStringLiteral("FallbackTracks"), config.fallbackTracks);
    settings.setValue(QStringLiteral("NotifyScreen"), config.notifyScreen);
    settings.setValue(QStringLiteral("NotifyPhone"), config.notifyPhone);
    settings.setValue(QStringLiteral("WatchStopped"), config.watchStopped);
    settings.endGroup();
    settings.sync();
}

void DeadAirWatchdog::setConfig(const Config &config)
{
    const bool wasEnabled = m_config.enabled;
    m_config = config;
    saveConfig(m_config);

    if (!m_config.enabled && m_tripped)
        endIncident(tr("Watchdog turned off during an incident"), false);

    if (!m_config.enabled) {
        m_silentSinceMs = m_stalledSinceMs = -1;
    } else if (!wasEnabled) {
        // Coming on mid-programme, give the station the benefit of the doubt:
        // start counting from now rather than from whatever stale timestamps
        // were left behind.
        m_silentSinceMs = m_stalledSinceMs = -1;
        m_lastTripEndMs = m_clock.elapsed();
        emit logMessage(tr("Dead-air watchdog armed: silence below %1 dBFS "
                           "for %2 s starts the fallback")
                            .arg(m_config.silenceDb, 0, 'f', 0)
                            .arg(m_config.tripSeconds));
    }
    emit stateChanged();
}

// ---------------------------------------------------------------------- feed

double DeadAirWatchdog::linearToDb(double linear)
{
    if (linear <= 0.000001)
        return -120.0;
    return 20.0 * std::log10(linear);
}

void DeadAirWatchdog::noteLevels(float left, float right)
{
    const double peak = qMax(double(left), double(right));
    m_levelDb = linearToDb(peak);
    m_levelSeen = true;
}

void DeadAirWatchdog::noteTransport(bool playing, qint64 positionMs,
                                    bool haveProgramme, bool levelsLive)
{
    m_playing = playing;
    m_positionMs = positionMs;
    m_haveProgramme = haveProgramme;
    m_levelsLive = levelsLive;
    if (playing)
        m_everOnAir = true;
}

void DeadAirWatchdog::noteOperatorStop()
{
    // Silence after a deliberate stop is the operator's silence, not a fault.
    m_operatorStopped = true;
    m_silentSinceMs = m_stalledSinceMs = -1;
}

void DeadAirWatchdog::noteOperatorPlay()
{
    m_operatorStopped = false;
    m_everOnAir = true;
}

void DeadAirWatchdog::setAutoTick(bool on)
{
    if (!m_timer)
        return;
    if (on)
        m_timer->start();
    else
        m_timer->stop();
}

// ----------------------------------------------------------- the state machine

void DeadAirWatchdog::tick(qint64 nowMs)
{
    if (!m_config.enabled)
        return;

    const qint64 tripMs = qint64(m_config.tripSeconds) * 1000;
    const qint64 recoverMs = qint64(m_config.recoverSeconds) * 1000;
    const qint64 rearmMs = qint64(m_config.rearmSeconds) * 1000;

    // Is the level feed trustworthy right now? Passthrough playback emits no
    // samples at all, so an absent feed is "cannot tell", never "silent". A
    // feed that dries up entirely is a wedged engine, and the stall detector
    // below is what catches that.
    if (m_levelSeen) {
        m_levelSeen = false;
        m_levelEverSeen = true;
        m_ticksWithoutLevel = 0;
    } else if (m_ticksWithoutLevel < 1000) {
        ++m_ticksWithoutLevel;
    }
    const bool levelFresh = m_levelsLive && m_levelEverSeen
                            && m_ticksWithoutLevel <= kLevelStaleTicks;

    // --- what the air sounds like -------------------------------------------
    bool silent = false;
    bool stalled = false;

    if (!m_haveProgramme) {
        // Nothing loaded, nothing scheduled: an idle XFB is not a dark
        // station, it is a station that has not been asked to do anything.
    } else if (!m_playing) {
        silent = m_config.watchStopped && m_everOnAir && !m_operatorStopped;
    } else {
        if (levelFresh && m_levelDb < m_config.silenceDb)
            silent = true;
        // Position frozen while claiming to play. The first tick after a
        // source change never strikes: m_lastPositionMs is reset below only
        // when it actually changes, and a fresh track starts at 0 against a
        // stored -1.
        if (m_positionMs >= 0 && m_positionMs == m_lastPositionMs)
            stalled = true;
    }

    m_lastPositionMs = m_positionMs;

    // --- run the clocks ------------------------------------------------------
    if (silent) {
        if (m_silentSinceMs < 0)
            m_silentSinceMs = nowMs;
    } else {
        m_silentSinceMs = -1;
    }

    if (stalled) {
        if (m_stalledSinceMs < 0)
            m_stalledSinceMs = nowMs;
    } else {
        m_stalledSinceMs = -1;
    }

    const bool healthy = !silent && !stalled;
    if (healthy) {
        if (m_healthySinceMs < 0)
            m_healthySinceMs = nowMs;
    } else {
        m_healthySinceMs = -1;
    }

    // --- an incident in progress ---------------------------------------------
    if (m_tripped) {
        // The fallback is playing (or the operator fixed the real source).
        // Either way, sustained healthy audio ends the incident.
        if (healthy && m_healthySinceMs >= 0
            && (nowMs - m_healthySinceMs) >= recoverMs) {
            endIncident(tr("Audio has been healthy for %1 s")
                            .arg(m_config.recoverSeconds), true);
            m_lastTripEndMs = nowMs;
        }
        return;
    }

    // --- cooldown -------------------------------------------------------------
    // Never trip in a loop: after an incident ends, the station gets the rearm
    // window to prove itself before the watchdog is willing to act again.
    if (m_lastTripEndMs >= 0 && (nowMs - m_lastTripEndMs) < rearmMs)
        return;

    // --- trip? ---------------------------------------------------------------
    if (m_silentSinceMs >= 0 && (nowMs - m_silentSinceMs) >= tripMs) {
        if (!m_playing) {
            trip(Reason::Stopped,
                 tr("Nothing has been playing for %1 s with a running order loaded")
                     .arg((nowMs - m_silentSinceMs) / 1000));
        } else {
            trip(Reason::Silence,
                 tr("Master level under %1 dBFS for %2 s (last peak %3 dBFS)")
                     .arg(m_config.silenceDb, 0, 'f', 0)
                     .arg((nowMs - m_silentSinceMs) / 1000)
                     .arg(m_levelDb, 0, 'f', 1));
        }
        return;
    }

    if (m_stalledSinceMs >= 0 && (nowMs - m_stalledSinceMs) >= tripMs) {
        trip(Reason::Stall,
             tr("Playback position frozen at %1 ms for %2 s while playing")
                 .arg(m_positionMs)
                 .arg((nowMs - m_stalledSinceMs) / 1000));
    }
}

void DeadAirWatchdog::trip(Reason reason, const QString &detail)
{
    m_tripped = true;
    m_lastReason = reason;
    m_incidentStarted = QDateTime::currentDateTime();
    m_incidentStartMs = m_clock.elapsed();
    m_silentSinceMs = m_stalledSinceMs = m_healthySinceMs = -1;
    m_lastIncident = detail;

    // The incident goes into the as-run log through AirLog, on the same
    // 'fallback' source every unclaimed path already uses, so the outage
    // shows up in the very report an advertiser reads.
    openIncidentRow(reason, detail);

    // A human-readable line in xfb.log. Deliberately not a debug print: this
    // is the sentence somebody reads at nine in the morning trying to work
    // out what happened at three.
    emit logMessage(tr("DEAD AIR at %1 — %2. Starting the fallback.")
                        .arg(m_incidentStarted.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")),
                             detail));

    if (m_config.notifyScreen) {
        emit alert(tr("Dead air"),
                   tr("%1\n\nXFB is starting the fallback material.").arg(detail),
                   true);
    }

    emit tripped(reason, detail);
    emit stateChanged();
}

void DeadAirWatchdog::noteFallbackStarted(bool ok, const QString &what)
{
    if (!m_tripped)
        return;

    if (ok) {
        m_lastIncident = tr("%1 — fallback: %2").arg(m_lastIncident, what);
        emit logMessage(tr("Dead-air fallback started: %1").arg(what));
    } else {
        // Worse than the outage: the rescue itself failed. Say so loudly, and
        // do not pretend the incident is handled.
        m_lastIncident = tr("%1 — FALLBACK FAILED: %2").arg(m_lastIncident, what);
        emit logMessage(tr("Dead-air fallback FAILED: %1").arg(what));
        if (m_config.notifyScreen) {
            emit alert(tr("Dead air — fallback failed"),
                       tr("XFB could not start the fallback material.\n\n%1\n\n"
                          "The station is off the air until somebody acts.").arg(what),
                       true);
        }
    }
    emit stateChanged();
}

void DeadAirWatchdog::clearIncident()
{
    if (!m_tripped)
        return;
    endIncident(tr("Cleared by the operator"), false);
    m_lastTripEndMs = m_clock.elapsed();
}

void DeadAirWatchdog::endIncident(const QString &detail, bool recoveredNaturally)
{
    if (!m_tripped)
        return;

    const qint64 lengthMs = m_incidentStartMs >= 0
                                ? (m_clock.elapsed() - m_incidentStartMs) : 0;
    closeIncidentRow(lengthMs);

    m_tripped = false;
    m_healthySinceMs = -1;
    m_silentSinceMs = m_stalledSinceMs = -1;

    emit logMessage(tr("Dead-air incident ended after %1 s — %2")
                        .arg(lengthMs / 1000).arg(detail));
    if (recoveredNaturally && m_config.notifyScreen) {
        emit alert(tr("Back on air"),
                   tr("Audio is flowing again after %1 s.").arg(lengthMs / 1000),
                   false);
    }
    emit recovered(detail);
    emit stateChanged();
}

// -------------------------------------------------------------------- as-run

QString DeadAirWatchdog::reasonWord(Reason reason)
{
    switch (reason) {
    case Reason::Silence: return tr("silence");
    case Reason::Stall:   return tr("stalled transport");
    case Reason::Stopped: return tr("nothing playing");
    case Reason::None:    break;
    }
    return tr("unknown");
}

void DeadAirWatchdog::openIncidentRow(Reason reason, const QString &detail)
{
    AirLog::Entry entry;
    // 'fallback' is the source the as-run log already uses for material no
    // library table claims — which is exactly what an outage is.
    entry.source = QStringLiteral("fallback");
    entry.artist = tr("Dead air");
    entry.title = tr("%1 — %2").arg(reasonWord(reason), detail);
    entry.autoMode = true;
    m_incidentAirHandle = AirLog::instance()->open(entry);
}

void DeadAirWatchdog::closeIncidentRow(qint64 playedMs)
{
    if (m_incidentAirHandle == 0)
        return;
    AirLog::instance()->close(m_incidentAirHandle, playedMs,
                              QStringLiteral("deadair"));
    m_incidentAirHandle = 0;
}

// -------------------------------------------------------------------- status

DeadAirWatchdog::AirState DeadAirWatchdog::airState() const
{
    if (m_tripped)
        return AirState::Fallback;
    if (!m_playing)
        return m_haveProgramme ? AirState::Silent : AirState::Off;
    if (m_stalledSinceMs >= 0)
        return AirState::Stalled;
    if (m_silentSinceMs >= 0)
        return AirState::Silent;
    return AirState::Playing;
}

qint64 DeadAirWatchdog::uptimeSeconds() const
{
    return m_clock.isValid() ? m_clock.elapsed() / 1000 : 0;
}

// ------------------------------------------------------------ fallback material

QStringList DeadAirWatchdog::audioSuffixes()
{
    // One list for the whole application; this used to keep its own copy.
    return AudioFormats::suffixes();
}

QStringList DeadAirWatchdog::tracksFromPlaylistFile(const QString &path)
{
    QStringList tracks;
    if (path.trimmed().isEmpty())
        return tracks;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return tracks;

    // The same <track> elements on_actionSave_Playlist writes. The overlap
    // and volume-line attributes are ignored here on purpose: a rescue
    // playlist is played straight, not performed.
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name().toString() != QLatin1String("track"))
            continue;
        const QString entry = xml.readElementText().trimmed();
        if (!entry.isEmpty() && QFileInfo::exists(entry))
            tracks << entry;
    }
    return tracks;
}

QStringList DeadAirWatchdog::tracksFromFolder(const QString &folder)
{
    QStringList tracks;
    if (folder.trimmed().isEmpty())
        return tracks;

    // Recursive now, and through symlinked folders: an evergreen folder that
    // is organised into subfolders used to look empty, and the watchdog then
    // had nothing to put on air.
    tracks = AudioFormats::findAudioFiles(folder);
    return tracks;
}

QStringList DeadAirWatchdog::resolveFallbackTracks(const Config &config)
{
    QStringList tracks = tracksFromPlaylistFile(config.fallbackPlaylist);

    if (tracks.isEmpty()) {
        tracks = tracksFromFolder(config.fallbackFolder);
        if (config.shuffleFolder && tracks.size() > 1) {
            for (int i = tracks.size() - 1; i > 0; --i) {
                const int j = int(QRandomGenerator::global()->bounded(i + 1));
                tracks.swapItemsAt(i, j);
            }
        }
    }

    if (tracks.size() > config.fallbackTracks)
        tracks = tracks.mid(0, config.fallbackTracks);
    return tracks;
}
