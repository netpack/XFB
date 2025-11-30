#include "AudioService.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

// Static member definitions
const QStringList AudioService::SUPPORTED_FORMATS = {
    "mp3", "wav", "ogg", "flac", "aac", "m4a", "wma"
};

const QMediaRecorder::Quality AudioService::DEFAULT_RECORDING_QUALITY = QMediaRecorder::Quality::NormalQuality;

AudioService::AudioService(QObject* parent)
    : BaseService(parent)
    , m_currentVolume(DEFAULT_VOLUME)
    , m_recordingQuality(DEFAULT_RECORDING_QUALITY)
    , m_recordingSampleRate(DEFAULT_SAMPLE_RATE)
    , m_recordingBitRate(DEFAULT_BIT_RATE)
{
}

AudioService::~AudioService()
{
    if (isRunning()) {
        shutdown();
    }
}

bool AudioService::doInitialize()
{
    qDebug() << "AudioService: Initializing (non-blocking mode)...";
    
    // Initialize in non-blocking mode - don't fail if audio is unavailable
    bool playerOk = initializeMediaPlayer();
    bool recorderOk = initializeMediaRecorder();
    
    if (!playerOk && !recorderOk) {
        qWarning() << "AudioService: No audio capabilities available, running in silent mode";
        // Don't emit error - just continue without audio
    } else {
        if (playerOk) {
            qDebug() << "AudioService: Media player initialized";
        }
        if (recorderOk) {
            qDebug() << "AudioService: Media recorder initialized";
        }
    }
    
    // Always try to initialize device monitoring (it's safe)
    initializeDeviceMonitoring();
    
    qDebug() << "AudioService: Initialization completed (audio available: " << (playerOk || recorderOk) << ")";
    return true; // Always return true - audio is optional
}

void AudioService::doShutdown()
{
    qDebug() << "AudioService: Shutting down...";
    
    // Stop any ongoing operations
    stop();
    if (isRecording()) {
        stopRecording();
    }
    
    // Clean up resources
    m_mediaPlayer.reset();
    m_audioOutput.reset();
    m_mediaRecorder.reset();
    m_audioInput.reset();
    m_captureSession.reset();
    
    qDebug() << "AudioService: Shutdown complete";
}

QString AudioService::getServiceName() const
{
    return "AudioService";
}

// Playback control methods
bool AudioService::play(const QUrl& mediaUrl)
{
    if (!isPlayerReady()) {
        emit audioError("Media player not ready");
        return false;
    }
    
    if (!isSupportedFormat(mediaUrl)) {
        emit audioError("Unsupported audio format");
        return false;
    }
    
    emit mediaLoading(mediaUrl);
    m_currentMediaUrl = mediaUrl;
    m_mediaPlayer->setSource(mediaUrl);
    m_mediaPlayer->play();
    
    return true;
}

void AudioService::pause()
{
    if (isPlayerReady() && isPlaying()) {
        m_mediaPlayer->pause();
    }
}

void AudioService::stop()
{
    if (isPlayerReady()) {
        m_mediaPlayer->stop();
        m_currentMediaUrl.clear();
    }
}

void AudioService::resume()
{
    if (isPlayerReady() && isPaused()) {
        m_mediaPlayer->play();
    }
}

// Volume control methods
bool AudioService::setVolume(int volume)
{
    int validatedVolume = validateVolume(volume);
    
    if (m_audioOutput) {
        m_audioOutput->setVolume(volumeToFloat(validatedVolume));
        m_currentVolume = validatedVolume;
        return true;
    }
    
    return false;
}

int AudioService::volume() const
{
    return m_currentVolume;
}

// Position control methods
void AudioService::setPosition(qint64 position)
{
    if (isPlayerReady()) {
        m_mediaPlayer->setPosition(position);
    }
}

qint64 AudioService::position() const
{
    return isPlayerReady() ? m_mediaPlayer->position() : 0;
}

qint64 AudioService::duration() const
{
    return isPlayerReady() ? m_mediaPlayer->duration() : -1;
}

// Device management methods
QList<QAudioDevice> AudioService::getAvailableOutputDevices() const
{
    return QMediaDevices::audioOutputs();
}

bool AudioService::setOutputDevice(const QAudioDevice& device)
{
    if (!device.isNull() && m_audioOutput) {
        m_audioOutput->setDevice(device);
        emit audioDevicesChanged();
        return true;
    }
    return false;
}

QAudioDevice AudioService::currentOutputDevice() const
{
    return m_audioOutput ? m_audioOutput->device() : QAudioDevice();
}

QList<QAudioDevice> AudioService::getAvailableInputDevices() const
{
    return QMediaDevices::audioInputs();
}

bool AudioService::setInputDevice(const QAudioDevice& device)
{
    if (!device.isNull() && m_audioInput) {
        m_audioInput->setDevice(device);
        emit audioDevicesChanged();
        return true;
    }
    return false;
}

QAudioDevice AudioService::currentInputDevice() const
{
    return m_audioInput ? m_audioInput->device() : QAudioDevice();
}

// Recording control methods
bool AudioService::startRecording(const QString& outputPath)
{
    if (!isRecorderReady()) {
        emit recordingError("Media recorder not ready");
        return false;
    }
    
    if (isRecording()) {
        emit recordingError("Recording already in progress");
        return false;
    }
    
    if (!validateRecordingPath(outputPath)) {
        emit recordingError("Invalid recording output path: " + outputPath);
        return false;
    }
    
    // Check if we have input devices available
    QList<QAudioDevice> inputDevices = getAvailableInputDevices();
    if (inputDevices.isEmpty()) {
        emit recordingError("No audio input devices available");
        return false;
    }
    
    // Ensure directory exists
    QFileInfo fileInfo(outputPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        emit recordingError("Failed to create recording directory: " + dir.absolutePath());
        return false;
    }
    
    m_currentRecordingPath = outputPath;
    m_mediaRecorder->setOutputLocation(QUrl::fromLocalFile(outputPath));
    
    if (!setupRecordingSettings()) {
        emit recordingError("Failed to setup recording settings");
        return false;
    }
    
    m_mediaRecorder->record();
    
    if (m_mediaRecorder->recorderState() == QMediaRecorder::RecordingState) {
        emit recordingStarted(outputPath);
        return true;
    } else {
        emit recordingError("Failed to start recording");
        return false;
    }
}

bool AudioService::stopRecording()
{
    if (!isRecorderReady()) {
        return false;
    }
    
    if (!isRecording() && !isRecordingPaused()) {
        return true; // Already stopped
    }
    
    qint64 duration = recordingDuration();
    QString outputPath = m_currentRecordingPath;
    
    m_mediaRecorder->stop();
    
    // Wait for the recorder to actually stop
    if (m_mediaRecorder->recorderState() == QMediaRecorder::StoppedState) {
        emit recordingStopped(outputPath, duration);
        m_currentRecordingPath.clear();
        return true;
    }
    
    return false;
}

bool AudioService::pauseRecording()
{
    if (!isRecorderReady() || !isRecording()) {
        return false;
    }
    
    m_mediaRecorder->pause();
    
    if (m_mediaRecorder->recorderState() == QMediaRecorder::PausedState) {
        emit recordingPaused();
        return true;
    }
    
    return false;
}

bool AudioService::resumeRecording()
{
    if (!isRecorderReady() || !isRecordingPaused()) {
        return false;
    }
    
    m_mediaRecorder->record();
    
    if (m_mediaRecorder->recorderState() == QMediaRecorder::RecordingState) {
        emit recordingResumed();
        return true;
    }
    
    return false;
}

// Recording settings methods
bool AudioService::setRecordingQuality(QMediaRecorder::Quality quality)
{
    if (!isRecorderReady()) {
        return false;
    }
    
    m_recordingQuality = quality;
    m_mediaRecorder->setQuality(quality);
    return true;
}

QMediaRecorder::Quality AudioService::recordingQuality() const
{
    return m_recordingQuality;
}

bool AudioService::setRecordingFormat(const QMediaFormat& format)
{
    if (!isRecorderReady()) {
        return false;
    }
    
    // Check if format is supported
    QList<QMediaFormat> supportedFormats = getSupportedRecordingFormats();
    bool isSupported = false;
    for (const auto& supportedFormat : supportedFormats) {
        if (supportedFormat.fileFormat() == format.fileFormat() &&
            supportedFormat.audioCodec() == format.audioCodec()) {
            isSupported = true;
            break;
        }
    }
    
    if (!isSupported) {
        emit recordingError("Unsupported recording format");
        return false;
    }
    
    m_recordingFormat = format;
    m_mediaRecorder->setMediaFormat(format);
    return true;
}

QMediaFormat AudioService::recordingFormat() const
{
    return m_recordingFormat;
}

QList<QMediaFormat> AudioService::getSupportedRecordingFormats() const
{
    if (!isRecorderReady()) {
        return {};
    }
    
    // Create a list of common supported formats
    QList<QMediaFormat> formats;
    
    // Add common audio formats
    QMediaFormat wavFormat;
    wavFormat.setFileFormat(QMediaFormat::Wave);
    wavFormat.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    formats.append(wavFormat);
    
    QMediaFormat mp3Format;
    mp3Format.setFileFormat(QMediaFormat::MP3);
    mp3Format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
    formats.append(mp3Format);
    
    QMediaFormat oggFormat;
    oggFormat.setFileFormat(QMediaFormat::Ogg);
    oggFormat.setAudioCodec(QMediaFormat::AudioCodec::Vorbis);
    formats.append(oggFormat);
    
    return formats;
}

bool AudioService::setRecordingSampleRate(int sampleRate)
{
    if (sampleRate <= 0) {
        return false;
    }
    
    m_recordingSampleRate = sampleRate;
    return true;
}

int AudioService::recordingSampleRate() const
{
    return m_recordingSampleRate;
}

bool AudioService::setRecordingBitRate(int bitRate)
{
    if (bitRate <= 0) {
        return false;
    }
    
    m_recordingBitRate = bitRate;
    return true;
}

int AudioService::recordingBitRate() const
{
    return m_recordingBitRate;
}

// State query methods
QMediaPlayer::PlaybackState AudioService::playbackState() const
{
    return isPlayerReady() ? m_mediaPlayer->playbackState() : QMediaPlayer::StoppedState;
}

bool AudioService::isPlaying() const
{
    return playbackState() == QMediaPlayer::PlayingState;
}

bool AudioService::isPaused() const
{
    return playbackState() == QMediaPlayer::PausedState;
}

bool AudioService::isStopped() const
{
    return playbackState() == QMediaPlayer::StoppedState;
}

QMediaRecorder::RecorderState AudioService::recordingState() const
{
    return isRecorderReady() ? m_mediaRecorder->recorderState() : QMediaRecorder::StoppedState;
}

bool AudioService::isRecording() const
{
    return recordingState() == QMediaRecorder::RecordingState;
}

bool AudioService::isRecordingPaused() const
{
    return recordingState() == QMediaRecorder::PausedState;
}

bool AudioService::isRecordingStopped() const
{
    return recordingState() == QMediaRecorder::StoppedState;
}

qint64 AudioService::recordingDuration() const
{
    return isRecorderReady() ? m_mediaRecorder->duration() : 0;
}

// Media information methods
QUrl AudioService::currentMedia() const
{
    return m_currentMediaUrl;
}

bool AudioService::isSupportedFormat(const QUrl& mediaUrl) const
{
    if (!mediaUrl.isValid()) {
        return false;
    }
    
    QString filePath = mediaUrl.isLocalFile() ? mediaUrl.toLocalFile() : mediaUrl.toString();
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return SUPPORTED_FORMATS.contains(extension);
}

// Private slot implementations
void AudioService::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    emit playbackStateChanged(state);
    
    if (state == QMediaPlayer::PlayingState && !m_currentMediaUrl.isEmpty()) {
        emit mediaLoaded(m_currentMediaUrl);
    }
}

void AudioService::onPositionChanged(qint64 position)
{
    emit positionChanged(position);
}

void AudioService::onDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void AudioService::onMediaError(QMediaPlayer::Error error, const QString& errorString)
{
    Q_UNUSED(error)
    emit audioError("Playback error: " + errorString);
}

void AudioService::onVolumeChanged(float volume)
{
    int volumePercent = volumeFromFloat(volume);
    if (volumePercent != m_currentVolume) {
        m_currentVolume = volumePercent;
        emit volumeChanged(volumePercent);
    }
}

void AudioService::onAudioDevicesChanged()
{
    emit audioDevicesChanged();
}

void AudioService::onRecordingStateChanged(QMediaRecorder::RecorderState state)
{
    emit recordingStateChanged(state);
}

void AudioService::onRecordingDurationChanged(qint64 duration)
{
    emit recordingDurationChanged(duration);
}

void AudioService::onRecordingError(QMediaRecorder::Error error, const QString& errorString)
{
    Q_UNUSED(error)
    emit recordingError("Recording error: " + errorString);
}

// Private helper methods
bool AudioService::initializeMediaPlayer()
{
    try {
        qDebug() << "AudioService: Attempting to initialize media player...";
        
        // Check if Qt Multimedia is available at all
        QList<QAudioDevice> audioDevices;
        try {
            audioDevices = QMediaDevices::audioOutputs();
        } catch (...) {
            qWarning() << "AudioService: Cannot enumerate audio devices - multimedia not available";
            return false;
        }
        
        if (audioDevices.isEmpty()) {
            qWarning() << "AudioService: No audio output devices found - skipping media player";
            return false;
        }
        
        // Try to create media player
        try {
            m_mediaPlayer = std::make_unique<QMediaPlayer>(this);
        } catch (...) {
            qWarning() << "AudioService: Failed to create QMediaPlayer - GStreamer issues";
            return false;
        }
        
        // Try to create audio output - this is where GStreamer usually fails
        try {
            m_audioOutput = std::make_unique<QAudioOutput>(audioDevices.first(), this);
        } catch (...) {
            qWarning() << "AudioService: Failed to create QAudioOutput with device, trying default";
            try {
                m_audioOutput = std::make_unique<QAudioOutput>(this);
            } catch (...) {
                qWarning() << "AudioService: Failed to create any QAudioOutput - GStreamer backend broken";
                m_mediaPlayer.reset();
                return false;
            }
        }
        
        // Try to connect them - this often fails with GStreamer issues
        try {
            m_mediaPlayer->setAudioOutput(m_audioOutput.get());
        } catch (...) {
            qWarning() << "AudioService: Failed to connect media player to audio output";
            m_mediaPlayer.reset();
            m_audioOutput.reset();
            return false;
        }
        
        // Connect signals only if everything worked
        connect(m_mediaPlayer.get(), &QMediaPlayer::playbackStateChanged,
                this, &AudioService::onPlaybackStateChanged);
        connect(m_mediaPlayer.get(), &QMediaPlayer::positionChanged,
                this, &AudioService::onPositionChanged);
        connect(m_mediaPlayer.get(), &QMediaPlayer::durationChanged,
                this, &AudioService::onDurationChanged);
        connect(m_mediaPlayer.get(), &QMediaPlayer::errorOccurred,
                this, &AudioService::onMediaError);
        
        connect(m_audioOutput.get(), &QAudioOutput::volumeChanged,
                this, &AudioService::onVolumeChanged);
        
        // Set initial volume
        setVolume(m_currentVolume);
        
        qDebug() << "AudioService: Media player initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "AudioService: Exception during media player init:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "AudioService: Unknown exception during media player init";
        return false;
    }
}

bool AudioService::initializeMediaRecorder()
{
    try {
        qDebug() << "AudioService: Attempting to initialize media recorder...";
        
        // Check if audio input devices are available
        QList<QAudioDevice> inputDevices;
        try {
            inputDevices = QMediaDevices::audioInputs();
        } catch (...) {
            qWarning() << "AudioService: Cannot enumerate audio input devices";
            return false;
        }
        
        if (inputDevices.isEmpty()) {
            qWarning() << "AudioService: No audio input devices found - recording disabled";
            return false;
        }
        
        // Try to create recorder components
        try {
            m_mediaRecorder = std::make_unique<QMediaRecorder>(this);
            m_captureSession = std::make_unique<QMediaCaptureSession>(this);
        } catch (...) {
            qWarning() << "AudioService: Failed to create recorder components";
            return false;
        }
        
        // Try to create audio input
        try {
            m_audioInput = std::make_unique<QAudioInput>(inputDevices.first(), this);
        } catch (...) {
            qWarning() << "AudioService: Failed to create audio input with device, trying default";
            try {
                m_audioInput = std::make_unique<QAudioInput>(this);
            } catch (...) {
                qWarning() << "AudioService: Failed to create any audio input";
                m_mediaRecorder.reset();
                m_captureSession.reset();
                return false;
            }
        }
        
        // Try to setup capture session
        try {
            m_captureSession->setRecorder(m_mediaRecorder.get());
            m_captureSession->setAudioInput(m_audioInput.get());
        } catch (...) {
            qWarning() << "AudioService: Failed to setup capture session";
            m_mediaRecorder.reset();
            m_captureSession.reset();
            m_audioInput.reset();
            return false;
        }
        
        // Connect recorder signals
        connect(m_mediaRecorder.get(), &QMediaRecorder::recorderStateChanged,
                this, &AudioService::onRecordingStateChanged);
        connect(m_mediaRecorder.get(), &QMediaRecorder::durationChanged,
                this, &AudioService::onRecordingDurationChanged);
        connect(m_mediaRecorder.get(), &QMediaRecorder::errorOccurred,
                this, &AudioService::onRecordingError);
        
        // Set default recording format
        m_recordingFormat = getDefaultRecordingFormat();
        m_mediaRecorder->setMediaFormat(m_recordingFormat);
        m_mediaRecorder->setQuality(m_recordingQuality);
        
        return true;
    } catch (const std::exception& e) {
        qDebug() << "AudioService: Failed to initialize media recorder:" << e.what();
        return false;
    }
}

void AudioService::initializeDeviceMonitoring()
{
    // Note: QMediaDevices doesn't have instance() method in Qt6
    // Device monitoring would need to be implemented differently
    // For now, we'll skip automatic device monitoring
    qDebug() << "AudioService: Device monitoring not implemented in this Qt6 version";
}

int AudioService::validateVolume(int volume) const
{
    return qBound(MIN_VOLUME, volume, MAX_VOLUME);
}

float AudioService::volumeToFloat(int volume) const
{
    return static_cast<float>(validateVolume(volume)) / 100.0f;
}

int AudioService::volumeFromFloat(float volume) const
{
    return static_cast<int>(qBound(0.0f, volume, 1.0f) * 100.0f);
}

bool AudioService::isPlayerReady() const
{
    return m_mediaPlayer && m_audioOutput;
}

bool AudioService::isRecorderReady() const
{
    return m_mediaRecorder && m_audioInput && m_captureSession;
}

bool AudioService::validateRecordingPath(const QString& outputPath) const
{
    if (outputPath.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(outputPath);
    
    // Check if the directory is writable
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }
    
    // Check if we can write to the directory
    QFileInfo dirInfo(dir.absolutePath());
    if (!dirInfo.isWritable()) {
        return false;
    }
    
    // Check file extension
    QString extension = fileInfo.suffix().toLower();
    if (extension.isEmpty()) {
        return false;
    }
    
    return true;
}

bool AudioService::setupRecordingSettings()
{
    if (!isRecorderReady()) {
        return false;
    }
    
    // Apply current settings
    m_mediaRecorder->setMediaFormat(m_recordingFormat);
    m_mediaRecorder->setQuality(m_recordingQuality);
    
    return true;
}

QMediaFormat AudioService::getDefaultRecordingFormat() const
{
    QMediaFormat format;
    
    // Set default format to WAV as it's widely supported
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    
    return format;
}