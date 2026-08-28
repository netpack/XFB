#include "FxPlayer.h"

#include <QAudioOutput>
#include <QDebug>

#include <utility>

#include "FxEngine.h"

FxPlayer::FxPlayer(QObject *parent)
    : QObject(parent)
{
    // Passthrough path
    m_qt = new QMediaPlayer(this);
    connectPassthrough(m_qt);

    // FX path (worker thread)
    m_engine = new FxEngine();
    m_engine->moveToThread(&m_engineThread);
    connect(&m_engineThread, &QThread::finished, m_engine, &QObject::deleteLater);
    m_engineThread.setObjectName(QStringLiteral("FxEngineThread"));
    m_engineThread.start();

    connect(m_engine, &FxEngine::positionChanged, this, [this](qint64 p) {
        if (m_mode == Mode::Fx) {
            m_fxPos = p;
            emit positionChanged(p);
        }
    });
    connect(m_engine, &FxEngine::durationChanged, this, [this](qint64 d) {
        if (m_mode == Mode::Fx) {
            m_fxDuration = d;
            emit durationChanged(d);
        }
    });
    connect(m_engine, &FxEngine::levels, this, [this](float l, float r) {
        if (m_mode == Mode::Fx)
            emit levels(l, r);
    });
    connect(m_engine, &FxEngine::pcmTap, this,
            [this](const QByteArray &pcm, int sampleRate, int channels) {
        // Nothing is emitted by the engine unless the tap is armed, so this
        // connection costs nothing while XFB is not on air.
        if (m_mode == Mode::Fx)
            emit pcmTap(pcm, sampleRate, channels);
    });
    connect(m_engine, &FxEngine::playbackFinished, this, [this]() {
        if (m_mode != Mode::Fx || m_fxState == QMediaPlayer::StoppedState)
            return;
        m_fxState = QMediaPlayer::StoppedState;
        emit mediaStatusChanged(QMediaPlayer::EndOfMedia);
        emit playbackStateChanged(QMediaPlayer::StoppedState);
    });
    connect(m_engine, &FxEngine::engineError, this, [this](const QString &msg) {
        if (m_mode != Mode::Fx)
            return;
        // Network streams have no passthrough fallback (QMediaPlayer cannot
        // play them) — report the error instead of switching.
        if (m_source.scheme().startsWith(QLatin1String("http"))) {
            qWarning() << "FxPlayer: stream failed:" << msg;
            if (m_fxState != QMediaPlayer::StoppedState) {
                m_fxState = QMediaPlayer::StoppedState;
                emit playbackStateChanged(QMediaPlayer::StoppedState);
            }
            emit errorOccurred(QMediaPlayer::ResourceError, msg);
            return;
        }
        qWarning() << "FxPlayer: FX engine failed, falling back to plain playback:" << msg;
        m_fxFailedForTrack = true;
        const QMediaPlayer::PlaybackState prev = m_fxState;
        const qint64 pos = m_fxPos;
        if (prev == QMediaPlayer::PlayingState || prev == QMediaPlayer::PausedState) {
            switchToPassthrough(prev, pos);
        } else {
            emit errorOccurred(QMediaPlayer::ResourceError, msg);
        }
    });
}

FxPlayer::~FxPlayer()
{
    // Blocking so the ffmpeg process and audio sink are torn down before
    // the thread exits (the engine thread never blocks on the GUI thread,
    // so this cannot deadlock).
    QMetaObject::invokeMethod(m_engine, &FxEngine::shutdown, Qt::BlockingQueuedConnection);
    m_engineThread.quit();
    m_engineThread.wait(3000);
}

template <typename F>
void FxPlayer::engineCall(F &&f)
{
    FxEngine *engine = m_engine;
    QMetaObject::invokeMethod(engine, [engine, f]() { f(engine); }, Qt::QueuedConnection);
}

void FxPlayer::connectPassthrough(QMediaPlayer *p)
{
    // Forward signals only from the player currently fronting playback:
    // after a gapless swap the retired instance stays alive as the standby
    // and must go quiet (p != m_qt).
    connect(p, &QMediaPlayer::positionChanged, this, [this, p](qint64 v) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit positionChanged(v);
    });
    connect(p, &QMediaPlayer::durationChanged, this, [this, p](qint64 d) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit durationChanged(d);
    });
    connect(p, &QMediaPlayer::sourceChanged, this, [this, p](const QUrl &u) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit sourceChanged(u);
    });
    connect(p, &QMediaPlayer::playbackStateChanged, this, [this, p](QMediaPlayer::PlaybackState s) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit playbackStateChanged(s);
    });
    connect(p, &QMediaPlayer::mediaStatusChanged, this, [this, p](QMediaPlayer::MediaStatus s) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit mediaStatusChanged(s);
    });
    connect(p, &QMediaPlayer::errorOccurred, this,
            [this, p](QMediaPlayer::Error e, const QString &msg) {
        if (p == m_qt && m_mode == Mode::Passthrough && !m_switching)
            emit errorOccurred(e, msg);
    });
}

bool FxPlayer::fxAvailable()
{
    return FxEngine::available();
}

namespace
{
bool isStreamUrl(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}
} // namespace

bool FxPlayer::wantFxFor(const QUrl &url) const
{
    if (!fxAvailable() || m_fxFailedForTrack)
        return false;
    // Network streams always go through the engine: the QMediaPlayer ffmpeg
    // backend does not reliably play endless Icecast/Shoutcast streams
    // (stuck in LoadingMedia), while the ffmpeg CLI handles them fine.
    if (isStreamUrl(url))
        return true;
    // Both the broadcast tap and loudness normalisation need the engine for
    // reasons of their own — the tap because only the engine can hand over
    // post-DSP PCM, normalisation because a positive gain cannot come from a
    // sink volume clamped to 0..1.
    return (m_params.anyActive() || m_preferEngine || m_pcmTap || m_loudnessActive)
        && url.isLocalFile();
}

void FxPlayer::setAudioOutput(QAudioOutput *output)
{
    m_output = output;
    m_qt->setAudioOutput(output);
    if (output) {
        connect(output, &QAudioOutput::volumeChanged, this, [this](float v) {
            engineCall([v](FxEngine *e) { e->setVolume(v); });
        });
        const float v = output->volume();
        engineCall([v](FxEngine *e) { e->setVolume(v); });
    }
}

void FxPlayer::setSource(const QUrl &source)
{
    const bool useHandoff = !source.isEmpty() && source == m_preparedUrl;
    const bool handoffInEngine = useHandoff && m_preparedInEngine;
    if (useHandoff) {
        m_preparedUrl.clear(); // consumed below (engine- or standby-side)
        m_preparedInEngine = false;
    } else {
        discardPrepared();
    }

    m_source = source;
    m_fxPos = 0;
    m_fxDuration = 0;
    m_fxFailedForTrack = false;

    if (wantFxFor(source)) {
        // Quiesce the passthrough player without leaking its signals
        m_switching = true;
        m_qt->stop();
        m_qt->setSource(QUrl());
        m_switching = false;

        m_mode = Mode::Fx;
        m_fxState = QMediaPlayer::StoppedState;
        const QString path = source.isLocalFile() ? source.toLocalFile() : source.toString();
        // The engine adopts its preloaded decoder when one is armed for
        // this path; otherwise this is the normal cold start.
        engineCall([path](FxEngine *e) { e->setSource(path); });

        emit sourceChanged(m_source);
        emit mediaStatusChanged(QMediaPlayer::LoadingMedia);
    } else {
        // Quiesce the FX engine
        if (m_mode == Mode::Fx) {
            engineCall([](FxEngine *e) { e->setSource(QString()); });
            m_fxState = QMediaPlayer::StoppedState;
        } else if (handoffInEngine) {
            // Prepared in the engine but playing passthrough (FX toggled
            // off since): the orphaned preload must not keep decoding.
            engineCall([](FxEngine *e) { e->cancelPreload(); });
        }
        m_mode = Mode::Passthrough;

        if (useHandoff && !handoffInEngine && m_qtStandby
                && (m_qtStandby->mediaStatus() == QMediaPlayer::LoadedMedia
                    || m_qtStandby->mediaStatus() == QMediaPlayer::BufferedMedia)) {
            // Gapless swap: the standby already holds this source fully
            // loaded, so play() starts without the media-load latency.
            m_switching = true;
            m_qt->stop();
            m_qt->setSource(QUrl());
            m_qt->setAudioOutput(nullptr);
            std::swap(m_qt, m_qtStandby);
            m_qt->setAudioOutput(m_output);
            m_switching = false;
            emit sourceChanged(m_source);
            emit durationChanged(m_qt->duration());
            emit mediaStatusChanged(m_qt->mediaStatus());
        } else {
            m_qt->setSource(source); // forwards sourceChanged & mediaStatus signals
        }
    }
}

void FxPlayer::prepareNext(const QUrl &url)
{
    if (url.isEmpty() || !url.isLocalFile())
        return;
    if (url == m_preparedUrl)
        return; // already armed
    discardPrepared();

    // Decide where the preload lives from what setSource() will pick for
    // this track (m_fxFailedForTrack is per-track state, so ignore it).
    const bool nextWantsFx = fxAvailable()
            && (m_params.anyActive() || m_preferEngine || m_pcmTap || m_loudnessActive);
    if (nextWantsFx) {
        const QString path = url.toLocalFile();
        engineCall([path](FxEngine *e) { e->preloadNext(path); });
        m_preparedInEngine = true;
    } else {
        if (!m_qtStandby) {
            m_qtStandby = new QMediaPlayer(this);
            connectPassthrough(m_qtStandby);
        }
        m_qtStandby->setSource(url); // loads without an audio output
        m_preparedInEngine = false;
    }
    m_preparedUrl = url;
}

bool FxPlayer::hasPreparedNext(const QUrl &url) const
{
    if (url.isEmpty() || url != m_preparedUrl)
        return false;
    if (m_preparedInEngine)
        return true; // optimistic: the engine cold-starts on a stale preload
    return m_qtStandby
           && (m_qtStandby->mediaStatus() == QMediaPlayer::LoadedMedia
               || m_qtStandby->mediaStatus() == QMediaPlayer::BufferedMedia);
}

void FxPlayer::resetAudioSink()
{
    engineCall([](FxEngine *e) { e->rebuildSink(); });
}

void FxPlayer::setNextCrossfade(qint64 fadeMs)
{
    engineCall([fadeMs](FxEngine *e) { e->setNextCrossfade(fadeMs); });
}

void FxPlayer::discardPrepared()
{
    if (m_preparedUrl.isEmpty())
        return;
    if (m_preparedInEngine)
        engineCall([](FxEngine *e) { e->cancelPreload(); });
    else if (m_qtStandby)
        m_qtStandby->setSource(QUrl());
    m_preparedUrl.clear();
    m_preparedInEngine = false;
}

void FxPlayer::play()
{
    if (m_mode == Mode::Fx) {
        if (m_fxState == QMediaPlayer::PlayingState)
            return;
        m_fxState = QMediaPlayer::PlayingState;
        engineCall([](FxEngine *e) { e->play(); });
        emit playbackStateChanged(QMediaPlayer::PlayingState);
        emit mediaStatusChanged(QMediaPlayer::BufferedMedia);
    } else {
        m_qt->play();
    }
}

void FxPlayer::pause()
{
    if (m_mode == Mode::Fx) {
        if (m_fxState != QMediaPlayer::PlayingState)
            return;
        m_fxState = QMediaPlayer::PausedState;
        engineCall([](FxEngine *e) { e->pause(); });
        emit playbackStateChanged(QMediaPlayer::PausedState);
    } else {
        m_qt->pause();
    }
}

void FxPlayer::stop()
{
    discardPrepared(); // an explicit stop invalidates any armed next track

    if (m_mode == Mode::Fx) {
        engineCall([](FxEngine *e) { e->stop(); });
        m_fxPos = 0;
        if (m_fxState != QMediaPlayer::StoppedState) {
            m_fxState = QMediaPlayer::StoppedState;
            emit playbackStateChanged(QMediaPlayer::StoppedState);
        }
    } else {
        m_qt->stop();
    }
}

void FxPlayer::setPosition(qint64 positionMs)
{
    if (m_mode == Mode::Fx) {
        m_fxPos = positionMs;
        engineCall([positionMs](FxEngine *e) { e->seek(positionMs); });
    } else {
        m_qt->setPosition(positionMs);
    }
}

qint64 FxPlayer::position() const
{
    return (m_mode == Mode::Fx) ? m_fxPos : m_qt->position();
}

qint64 FxPlayer::duration() const
{
    return (m_mode == Mode::Fx) ? m_fxDuration : m_qt->duration();
}

QMediaPlayer::PlaybackState FxPlayer::playbackState() const
{
    return (m_mode == Mode::Fx) ? m_fxState : m_qt->playbackState();
}

void FxPlayer::setFxParams(const FxParams &params)
{
    m_params = params;
    engineCall([params](FxEngine *e) { e->setParams(params); });

    applyModeForCurrentSource();
}

void FxPlayer::setPcmTapEnabled(bool enabled)
{
    if (m_pcmTap == enabled)
        return;
    m_pcmTap = enabled;

    engineCall([enabled](FxEngine *e) { e->setPcmTapEnabled(enabled); });

    // A track already playing through the plain QMediaPlayer produces no
    // tap, so going on air has to move it into the engine (and coming off
    // air may hand it back) — the same live switch setFxParams performs.
    applyModeForCurrentSource();
}

void FxPlayer::applyModeForCurrentSource()
{
    const bool wantFx = wantFxFor(m_source) && !m_source.isEmpty();
    const bool isFx = (m_mode == Mode::Fx);
    if (wantFx == isFx)
        return;

    if (wantFx) {
        const QMediaPlayer::PlaybackState state = m_qt->playbackState();
        const qint64 pos = m_qt->position();
        switchToFx(state, pos);
    } else {
        const QMediaPlayer::PlaybackState state = m_fxState;
        const qint64 pos = m_fxPos;
        switchToPassthrough(state, pos);
    }
}

void FxPlayer::setLoudnessActive(bool on)
{
    if (m_loudnessActive == on)
        return;
    m_loudnessActive = on;

    if (!on) {
        // Leave nothing behind in the engine: a later stream URL still goes
        // through it, and it must not inherit the last track's gain.
        engineCall([](FxEngine *e) { e->setLoudnessGainDb(0.0, true); });
    } else {
        const double g = m_loudnessGainDb;
        engineCall([g](FxEngine *e) { e->setLoudnessGainDb(g, true); });
    }

    // Switching normalisation on or off changes which path a local file
    // takes, exactly as toggling an FX parameter does.
    const bool wantFx = wantFxFor(m_source) && !m_source.isEmpty();
    const bool isFx = (m_mode == Mode::Fx);
    if (wantFx == isFx)
        return;

    if (wantFx)
        switchToFx(m_qt->playbackState(), m_qt->position());
    else
        switchToPassthrough(m_fxState, m_fxPos);
}

void FxPlayer::setLoudnessGainDb(double gainDb, bool immediate)
{
    m_loudnessGainDb = gainDb;
    const double g = m_loudnessActive ? gainDb : 0.0;
    engineCall([g, immediate](FxEngine *e) { e->setLoudnessGainDb(g, immediate); });
}

void FxPlayer::setLimiter(bool enabled, double ceilingDbTp)
{
    m_limiterOn = enabled;
    m_limiterCeilingDbTp = ceilingDbTp;
    engineCall([enabled, ceilingDbTp](FxEngine *e) {
        e->setLimiter(enabled, ceilingDbTp);
    });
}

void FxPlayer::setDjFx(double filterAmount, double echoAmount)
{
    engineCall([filterAmount, echoAmount](FxEngine *e) {
        e->setDjFx(filterAmount, echoAmount);
    });
}

void FxPlayer::scratchBegin()
{
    if (m_mode == Mode::Fx)
        engineCall([](FxEngine *e) { e->scratchBegin(); });
}

void FxPlayer::scratchMove(double targetRate)
{
    if (m_mode == Mode::Fx)
        engineCall([targetRate](FxEngine *e) { e->scratchMove(targetRate); });
}

void FxPlayer::scratchEnd()
{
    if (m_mode == Mode::Fx)
        engineCall([](FxEngine *e) { e->scratchEnd(); });
}

void FxPlayer::djBrake()
{
    if (m_mode == Mode::Fx)
        engineCall([](FxEngine *e) { e->djBrake(); });
}

void FxPlayer::djBackspin()
{
    if (m_mode == Mode::Fx)
        engineCall([](FxEngine *e) { e->djBackspin(); });
}

void FxPlayer::switchToFx(QMediaPlayer::PlaybackState resumeState, qint64 resumePos)
{
    m_switching = true;
    m_qt->stop();
    m_qt->setSource(QUrl());
    m_switching = false;

    m_mode = Mode::Fx;
    m_fxPos = resumePos;
    m_fxState = resumeState;

    const QString path = m_source.isLocalFile() ? m_source.toLocalFile() : m_source.toString();
    engineCall([path](FxEngine *e) { e->setSource(path); });
    if (resumeState == QMediaPlayer::PlayingState) {
        engineCall([resumePos](FxEngine *e) {
            if (resumePos > 0)
                e->seek(resumePos);
            e->play();
        });
    } else if (resumeState == QMediaPlayer::PausedState) {
        engineCall([resumePos](FxEngine *e) { e->seek(resumePos); });
    } else {
        m_fxState = QMediaPlayer::StoppedState;
    }
}

void FxPlayer::switchToPassthrough(QMediaPlayer::PlaybackState resumeState, qint64 resumePos)
{
    engineCall([](FxEngine *e) { e->stop(); });
    m_mode = Mode::Passthrough;
    m_fxState = QMediaPlayer::StoppedState;

    m_switching = true;
    m_qt->setSource(m_source);
    if (resumeState == QMediaPlayer::PlayingState) {
        m_qt->play();
        if (resumePos > 0)
            m_qt->setPosition(resumePos);
    } else if (resumeState == QMediaPlayer::PausedState) {
        m_qt->play();
        if (resumePos > 0)
            m_qt->setPosition(resumePos);
        m_qt->pause();
    }
    m_switching = false;
}
