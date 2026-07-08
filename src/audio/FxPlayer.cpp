#include "FxPlayer.h"

#include <QAudioOutput>
#include <QDebug>

#include "FxEngine.h"

FxPlayer::FxPlayer(QObject *parent)
    : QObject(parent)
{
    // Passthrough path
    m_qt = new QMediaPlayer(this);

    connect(m_qt, &QMediaPlayer::positionChanged, this, [this](qint64 p) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit positionChanged(p);
    });
    connect(m_qt, &QMediaPlayer::durationChanged, this, [this](qint64 d) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit durationChanged(d);
    });
    connect(m_qt, &QMediaPlayer::sourceChanged, this, [this](const QUrl &u) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit sourceChanged(u);
    });
    connect(m_qt, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit playbackStateChanged(s);
    });
    connect(m_qt, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus s) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit mediaStatusChanged(s);
    });
    connect(m_qt, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error e, const QString &msg) {
        if (m_mode == Mode::Passthrough && !m_switching)
            emit errorOccurred(e, msg);
    });

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
    return (m_params.anyActive() || m_preferEngine) && url.isLocalFile();
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
        engineCall([path](FxEngine *e) { e->setSource(path); });

        emit sourceChanged(m_source);
        emit mediaStatusChanged(QMediaPlayer::LoadingMedia);
    } else {
        // Quiesce the FX engine
        if (m_mode == Mode::Fx) {
            engineCall([](FxEngine *e) { e->setSource(QString()); });
            m_fxState = QMediaPlayer::StoppedState;
        }
        m_mode = Mode::Passthrough;
        m_qt->setSource(source); // forwards sourceChanged & mediaStatus signals
    }
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
