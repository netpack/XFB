#include "StreamService.h"

#include "../secretstore.h"

#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

namespace {
constexpr int kSilenceTickMs = 250;
const char *kGroup = "Stream";
} // namespace

StreamService::StreamService(QObject *parent)
    : QObject(parent)
{
    m_mounts = loadMounts();

    m_silenceTimer = new QTimer(this);
    m_silenceTimer->setInterval(kSilenceTickMs);
    connect(m_silenceTimer, &QTimer::timeout, this, &StreamService::feedSilence);
}

StreamService::~StreamService()
{
    tearDownFeeds();
}

// ------------------------------------------------------------------ settings

QString StreamService::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

StreamService::Mount StreamService::defaultMount()
{
    Mount mount;
    mount.name = QObject::tr("Main stream");
    mount.server.host = QStringLiteral("localhost");
    mount.server.port = 8000;
    mount.server.mount = QStringLiteral("/stream");
    mount.server.user = QStringLiteral("source");
    mount.server.contentType =
        StreamEncoder::contentTypeFor(StreamEncoder::Codec::Mp3);
    mount.encoder.codec = StreamEncoder::Codec::Mp3;
    mount.encoder.bitrateKbps = 128;
    return mount;
}

QVector<StreamService::Mount> StreamService::loadMounts()
{
    QVector<Mount> mounts;
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));

    const int count = settings.beginReadArray(QStringLiteral("mounts"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        Mount mount;
        mount.name = settings.value(QStringLiteral("Name")).toString();
        mount.enabled = settings.value(QStringLiteral("Enabled"), true).toBool();

        mount.server.host = settings.value(QStringLiteral("Host"),
                                           QStringLiteral("localhost")).toString();
        mount.server.port =
            static_cast<quint16>(settings.value(QStringLiteral("Port"), 8000).toUInt());
        mount.server.mount = settings.value(QStringLiteral("Mount"),
                                            QStringLiteral("/stream")).toString();
        mount.server.user = settings.value(QStringLiteral("User"),
                                           QStringLiteral("source")).toString();
        // Sealed the same way as every other credential in xfb.conf.
        mount.server.password =
            SecretStore::open(settings.value(QStringLiteral("Password")).toString());
        mount.server.usePut = settings.value(QStringLiteral("UsePut"), false).toBool();
        mount.server.name = settings.value(QStringLiteral("StationName")).toString();
        mount.server.genre = settings.value(QStringLiteral("Genre")).toString();
        mount.server.description = settings.value(QStringLiteral("Description")).toString();
        mount.server.url = settings.value(QStringLiteral("Url")).toString();
        mount.server.isPublic = settings.value(QStringLiteral("Public"), false).toBool();

        mount.encoder.codec = StreamEncoder::codecFromName(
            settings.value(QStringLiteral("Codec"), QStringLiteral("mp3")).toString());
        mount.encoder.bitrateKbps =
            settings.value(QStringLiteral("Bitrate"), 128).toInt();

        // Derived rather than stored: a mount whose Content-Type disagrees
        // with its codec plays as noise, and that is not worth making
        // configurable.
        mount.server.contentType = StreamEncoder::contentTypeFor(mount.encoder.codec);
        mount.server.bitrateKbps = mount.encoder.bitrateKbps;

        if (mount.name.isEmpty())
            mount.name = mount.server.mount;
        mounts.append(mount);
    }
    settings.endArray();
    settings.endGroup();
    return mounts;
}

void StreamService::saveMounts(const QVector<Mount> &mounts)
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));

    settings.beginWriteArray(QStringLiteral("mounts"), mounts.size());
    for (int i = 0; i < mounts.size(); ++i) {
        settings.setArrayIndex(i);
        const Mount &mount = mounts.at(i);
        settings.setValue(QStringLiteral("Name"), mount.name);
        settings.setValue(QStringLiteral("Enabled"), mount.enabled);
        settings.setValue(QStringLiteral("Host"), mount.server.host);
        settings.setValue(QStringLiteral("Port"), mount.server.port);
        settings.setValue(QStringLiteral("Mount"), mount.server.mount);
        settings.setValue(QStringLiteral("User"), mount.server.user);
        settings.setValue(QStringLiteral("Password"),
                          SecretStore::seal(mount.server.password));
        settings.setValue(QStringLiteral("UsePut"), mount.server.usePut);
        settings.setValue(QStringLiteral("StationName"), mount.server.name);
        settings.setValue(QStringLiteral("Genre"), mount.server.genre);
        settings.setValue(QStringLiteral("Description"), mount.server.description);
        settings.setValue(QStringLiteral("Url"), mount.server.url);
        settings.setValue(QStringLiteral("Public"), mount.server.isPublic);
        settings.setValue(QStringLiteral("Codec"),
                          StreamEncoder::codecName(mount.encoder.codec));
        settings.setValue(QStringLiteral("Bitrate"), mount.encoder.bitrateKbps);
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();

    // The file now holds stream passwords: owner-only, like the rest of it.
    SecretStore::restrictFile(configPath());
}

bool StreamService::autoStartEnabled()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    const bool on = settings.value(QStringLiteral("AutoStart"), false).toBool();
    settings.endGroup();
    return on;
}

void StreamService::setAutoStartEnabled(bool on)
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    settings.setValue(QStringLiteral("AutoStart"), on);
    settings.endGroup();
}

// --------------------------------------------------------------- mount state

void StreamService::setMounts(const QVector<Mount> &mounts)
{
    m_mounts = mounts;
    saveMounts(m_mounts);
    if (m_active) {
        // Host, codec and credentials are all handshake-time decisions, so
        // an edit means re-establishing the connections.
        tearDownFeeds();
        buildFeeds();
    }
    emit statusChanged();
}

int StreamService::connectedCount() const
{
    int count = 0;
    for (const Feed &feed : m_feeds) {
        if (feed.source && feed.source->isConnected())
            ++count;
    }
    return count;
}

QVector<IcecastSource::State> StreamService::mountStates() const
{
    QVector<IcecastSource::State> states;
    states.reserve(m_mounts.size());
    for (int i = 0; i < m_mounts.size(); ++i) {
        if (i < m_feeds.size() && m_feeds.at(i).source)
            states.append(m_feeds.at(i).source->state());
        else
            states.append(IcecastSource::State::Disconnected);
    }
    return states;
}

QVector<QString> StreamService::mountErrors() const
{
    QVector<QString> errors;
    errors.reserve(m_mounts.size());
    for (int i = 0; i < m_mounts.size(); ++i) {
        if (i < m_feeds.size() && m_feeds.at(i).source)
            errors.append(m_feeds.at(i).source->lastError());
        else
            errors.append(QString());
    }
    return errors;
}

// -------------------------------------------------------------- start / stop

void StreamService::start()
{
    if (m_active)
        return;

    bool anyEnabled = false;
    for (const Mount &mount : m_mounts) {
        if (mount.enabled) {
            anyEnabled = true;
            break;
        }
    }
    if (m_mounts.isEmpty() || !anyEnabled) {
        emit logMessage(tr("There is no enabled mount to stream to."));
        return;
    }

    m_active = true;
    m_bytesFed = 0;
    m_clock.start();
    buildFeeds();
    m_silenceTimer->start();
    emit activeChanged(true);
    emit statusChanged();
}

void StreamService::stop()
{
    if (!m_active)
        return;
    m_active = false;
    m_silenceTimer->stop();
    tearDownFeeds();
    emit activeChanged(false);
    emit statusChanged();
    emit logMessage(tr("Streaming stopped."));
}

void StreamService::buildFeeds()
{
    m_feeds.clear();
    m_feeds.reserve(m_mounts.size());

    for (const Mount &mount : m_mounts) {
        Feed feed;
        if (!mount.enabled) {
            m_feeds.append(feed);   // keep the indices aligned with m_mounts
            continue;
        }

        const QString label = mount.name.isEmpty() ? mount.server.mount : mount.name;

        feed.encoder = new StreamEncoder(this);
        StreamEncoder::Config encoderConfig = mount.encoder;
        encoderConfig.sampleRate = m_sampleRate;
        encoderConfig.channels = m_channels;
        feed.encoder->setConfig(encoderConfig);

        feed.source = new IcecastSource(this);
        IcecastSource::Config serverConfig = mount.server;
        serverConfig.contentType = StreamEncoder::contentTypeFor(mount.encoder.codec);
        serverConfig.bitrateKbps = mount.encoder.bitrateKbps;
        serverConfig.sampleRate = m_sampleRate;
        serverConfig.channels = m_channels;
        feed.source->setConfig(serverConfig);

        // Encoded bytes go straight to the socket; nothing in between needs
        // to see them.
        connect(feed.encoder, &StreamEncoder::encoded,
                feed.source, &IcecastSource::writeAudio);

        connect(feed.encoder, &StreamEncoder::logMessage, this,
                [this, label](const QString &message) {
            emit logMessage(QStringLiteral("[%1] %2").arg(label, message));
        });
        connect(feed.source, &IcecastSource::logMessage, this,
                [this, label](const QString &message) {
            emit logMessage(QStringLiteral("[%1] %2").arg(label, message));
        });
        connect(feed.source, &IcecastSource::stateChanged, this,
                [this](IcecastSource::State) { emit statusChanged(); });

        IcecastSource *source = feed.source;
        connect(feed.source, &IcecastSource::connected, this, [this, source]() {
            // Whatever is on air right now, not whatever was on when this
            // mount last connected.
            if (!m_nowPlaying.isEmpty())
                source->setMetadata(m_nowPlaying);
        });

        feed.encoder->start();
        feed.source->connectToServer();
        m_feeds.append(feed);
    }
}

void StreamService::tearDownFeeds()
{
    for (Feed &feed : m_feeds) {
        if (feed.source) {
            feed.source->disconnectFromServer();
            feed.source->deleteLater();
        }
        if (feed.encoder) {
            feed.encoder->stop();
            feed.encoder->deleteLater();
        }
    }
    m_feeds.clear();
}

// ---------------------------------------------------------------- audio path

void StreamService::feedPcm(const QByteArray &pcm, int sampleRate, int channels)
{
    if (!m_active || pcm.isEmpty())
        return;

    if (sampleRate != m_sampleRate || channels != m_channels) {
        // The engine's tap format is fixed, so this only happens if it is
        // ever changed underneath us — rebuild rather than send noise.
        m_sampleRate = sampleRate;
        m_channels = channels;
        emit logMessage(tr("Tap format changed to %1 Hz, %2 channels; "
                           "restarting the encoders.")
                            .arg(sampleRate).arg(channels));
        tearDownFeeds();
        buildFeeds();
        m_bytesFed = 0;
        m_clock.restart();
    }

    m_bytesFed += pcm.size();
    for (const Feed &feed : m_feeds) {
        if (feed.encoder)
            feed.encoder->writePcm(pcm);
    }
}

void StreamService::feedSilence()
{
    if (!m_active || m_feeds.isEmpty())
        return;

    const qint64 bytesPerSecond =
        static_cast<qint64>(m_sampleRate) * m_channels * 2;
    if (bytesPerSecond <= 0)
        return;

    // The sink is filled ahead of real time, so in normal playback the tap
    // runs *ahead* of the clock and there is nothing to do here.
    const qint64 expected = m_clock.elapsed() * bytesPerSecond / 1000;
    qint64 deficit = expected - m_bytesFed;
    if (deficit < kSilenceThresholdMs * bytesPerSecond / 1000)
        return;

    const qint64 cap = kMaxSilenceBurstMs * bytesPerSecond / 1000;
    if (deficit > cap) {
        // A long gap (playback stopped for minutes) must not turn into a
        // huge write; skip the clock forward and carry on from here.
        m_bytesFed += deficit - cap;
        deficit = cap;
    }

    // Whole frames only: half a frame swaps the channels for everything after.
    const qint64 frameBytes = static_cast<qint64>(m_channels) * 2;
    deficit -= deficit % frameBytes;
    if (deficit <= 0)
        return;

    const QByteArray silence(static_cast<int>(deficit), '\0');
    m_bytesFed += deficit;
    for (const Feed &feed : m_feeds) {
        if (feed.encoder)
            feed.encoder->writePcm(silence);
    }
}

// ------------------------------------------------------------------ metadata

void StreamService::setNowPlaying(const QString &artist, const QString &title)
{
    // Icecast shows one string, and the convention every player and every
    // directory expects is "Artist - Title".
    QString song = title.trimmed();
    const QString cleanArtist = artist.trimmed();
    if (!cleanArtist.isEmpty() && !song.isEmpty())
        song = cleanArtist + QStringLiteral(" - ") + song;
    else if (!cleanArtist.isEmpty())
        song = cleanArtist;

    if (song.isEmpty() || song == m_nowPlaying)
        return;
    m_nowPlaying = song;
    emit nowPlayingChanged(song);

    if (!m_active)
        return;
    for (const Feed &feed : m_feeds) {
        if (feed.source)
            feed.source->setMetadata(song);
    }
}
