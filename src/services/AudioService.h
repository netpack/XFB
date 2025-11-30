#ifndef AUDIOSERVICE_H
#define AUDIOSERVICE_H

#include "BaseService.h"
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QAudioInput>
#include <QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QAudioDevice>
#include <QMediaFormat>
#include <QUrl>
#include <QList>
#include <memory>

/**
 * @brief Service for managing audio playback and device operations
 * 
 * The AudioService provides a high-level interface for all audio-related
 * operations in XFB. It handles playback control and device management
 * while abstracting the underlying Qt Multimedia complexity.
 * 
 * Features:
 * - Audio playback control (play, pause, stop)
 * - Audio recording with format and quality selection
 * - Volume control with range validation
 * - Audio device enumeration and selection (input/output)
 * - Audio format handling and validation
 * - Comprehensive error handling and reporting
 * 
 * @example
 * @code
 * AudioService* audioService = new AudioService(this);
 * connect(audioService, &AudioService::playbackStateChanged,
 *         this, &MainWindow::onPlaybackStateChanged);
 * audioService->initialize();
 * audioService->play(QUrl::fromLocalFile("/path/to/music.mp3"));
 * @endcode
 * 
 * @see QMediaPlayer, QAudioOutput, QMediaDevices
 * @since XFB 2.0
 */
class AudioService : public BaseService
{
    Q_OBJECT

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    // Playback control
    /**
     * @brief Start playing audio from the specified URL
     * @param mediaUrl URL of the media file to play
     * @return true if playback started successfully
     */
    bool play(const QUrl& mediaUrl);

    /**
     * @brief Pause current playback
     */
    void pause();

    /**
     * @brief Stop current playback
     */
    void stop();

    /**
     * @brief Resume playback if paused
     */
    void resume();

    // Volume control
    /**
     * @brief Set the playback volume
     * @param volume Volume level (0-100)
     * @return true if volume was set successfully
     */
    bool setVolume(int volume);

    /**
     * @brief Get the current volume level
     * @return Current volume (0-100)
     */
    int volume() const;

    // Position control
    /**
     * @brief Set the playback position
     * @param position Position in milliseconds
     */
    void setPosition(qint64 position);

    /**
     * @brief Get the current playback position
     * @return Current position in milliseconds
     */
    qint64 position() const;

    /**
     * @brief Get the duration of the current media
     * @return Duration in milliseconds, or -1 if unknown
     */
    qint64 duration() const;

    // Device management
    /**
     * @brief Get list of available audio output devices
     * @return List of available output devices
     */
    QList<QAudioDevice> getAvailableOutputDevices() const;

    /**
     * @brief Set the audio output device
     * @param device The audio device to use for output
     * @return true if device was set successfully
     */
    bool setOutputDevice(const QAudioDevice& device);

    /**
     * @brief Get the currently selected output device
     * @return Current output device
     */
    QAudioDevice currentOutputDevice() const;

    /**
     * @brief Get list of available audio input devices
     * @return List of available input devices
     */
    QList<QAudioDevice> getAvailableInputDevices() const;

    /**
     * @brief Set the audio input device for recording
     * @param device The audio device to use for input
     * @return true if device was set successfully
     */
    bool setInputDevice(const QAudioDevice& device);

    /**
     * @brief Get the currently selected input device
     * @return Current input device
     */
    QAudioDevice currentInputDevice() const;

    // Recording control
    /**
     * @brief Start recording audio to the specified file
     * @param outputPath Path where the recording will be saved
     * @return true if recording started successfully
     */
    bool startRecording(const QString& outputPath);

    /**
     * @brief Stop current recording
     * @return true if recording was stopped successfully
     */
    bool stopRecording();

    /**
     * @brief Pause current recording
     * @return true if recording was paused successfully
     */
    bool pauseRecording();

    /**
     * @brief Resume paused recording
     * @return true if recording was resumed successfully
     */
    bool resumeRecording();

    // Recording settings
    /**
     * @brief Set the recording quality
     * @param quality Recording quality level
     * @return true if quality was set successfully
     */
    bool setRecordingQuality(QMediaRecorder::Quality quality);

    /**
     * @brief Get the current recording quality
     * @return Current recording quality
     */
    QMediaRecorder::Quality recordingQuality() const;

    /**
     * @brief Set the recording format
     * @param format Media format for recording
     * @return true if format was set successfully
     */
    bool setRecordingFormat(const QMediaFormat& format);

    /**
     * @brief Get the current recording format
     * @return Current recording format
     */
    QMediaFormat recordingFormat() const;

    /**
     * @brief Get list of supported recording formats
     * @return List of supported formats for recording
     */
    QList<QMediaFormat> getSupportedRecordingFormats() const;

    /**
     * @brief Set the recording sample rate
     * @param sampleRate Sample rate in Hz
     * @return true if sample rate was set successfully
     */
    bool setRecordingSampleRate(int sampleRate);

    /**
     * @brief Get the current recording sample rate
     * @return Current sample rate in Hz
     */
    int recordingSampleRate() const;

    /**
     * @brief Set the recording bit rate
     * @param bitRate Bit rate in bits per second
     * @return true if bit rate was set successfully
     */
    bool setRecordingBitRate(int bitRate);

    /**
     * @brief Get the current recording bit rate
     * @return Current bit rate in bits per second
     */
    int recordingBitRate() const;

    // State queries
    /**
     * @brief Get the current playback state
     * @return Current playback state
     */
    QMediaPlayer::PlaybackState playbackState() const;

    /**
     * @brief Check if audio is currently playing
     * @return true if audio is playing
     */
    bool isPlaying() const;

    /**
     * @brief Check if audio is paused
     * @return true if audio is paused
     */
    bool isPaused() const;

    /**
     * @brief Check if audio is stopped
     * @return true if audio is stopped
     */
    bool isStopped() const;

    /**
     * @brief Get the current recording state
     * @return Current recording state
     */
    QMediaRecorder::RecorderState recordingState() const;

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    bool isRecording() const;

    /**
     * @brief Check if recording is paused
     * @return true if recording is paused
     */
    bool isRecordingPaused() const;

    /**
     * @brief Check if recording is stopped
     * @return true if recording is stopped
     */
    bool isRecordingStopped() const;

    /**
     * @brief Get the current recording duration
     * @return Recording duration in milliseconds
     */
    qint64 recordingDuration() const;

    // Media information
    /**
     * @brief Get the currently loaded media URL
     * @return Current media URL, or empty URL if none loaded
     */
    QUrl currentMedia() const;

    /**
     * @brief Check if the specified URL represents a supported audio format
     * @param mediaUrl URL to check
     * @return true if the format is supported
     */
    bool isSupportedFormat(const QUrl& mediaUrl) const;

signals:
    /**
     * @brief Emitted when playback state changes
     * @param state New playback state
     */
    void playbackStateChanged(QMediaPlayer::PlaybackState state);

    /**
     * @brief Emitted when playback position changes
     * @param position Current position in milliseconds
     */
    void positionChanged(qint64 position);

    /**
     * @brief Emitted when media duration becomes available
     * @param duration Duration in milliseconds
     */
    void durationChanged(qint64 duration);

    /**
     * @brief Emitted when volume changes
     * @param volume New volume level (0-100)
     */
    void volumeChanged(int volume);

    /**
     * @brief Emitted when an audio error occurs
     * @param error Error description
     */
    void audioError(const QString& error);

    /**
     * @brief Emitted when media loading starts
     * @param mediaUrl URL of the media being loaded
     */
    void mediaLoading(const QUrl& mediaUrl);

    /**
     * @brief Emitted when media is successfully loaded
     * @param mediaUrl URL of the loaded media
     */
    void mediaLoaded(const QUrl& mediaUrl);

    /**
     * @brief Emitted when available audio devices change
     */
    void audioDevicesChanged();

    /**
     * @brief Emitted when recording state changes
     * @param state New recording state
     */
    void recordingStateChanged(QMediaRecorder::RecorderState state);

    /**
     * @brief Emitted when recording duration changes
     * @param duration Current recording duration in milliseconds
     */
    void recordingDurationChanged(qint64 duration);

    /**
     * @brief Emitted when a recording error occurs
     * @param error Error description
     */
    void recordingError(const QString& error);

    /**
     * @brief Emitted when recording starts
     * @param outputPath Path where recording is being saved
     */
    void recordingStarted(const QString& outputPath);

    /**
     * @brief Emitted when recording stops
     * @param outputPath Path where recording was saved
     * @param duration Total recording duration in milliseconds
     */
    void recordingStopped(const QString& outputPath, qint64 duration);

    /**
     * @brief Emitted when recording is paused
     */
    void recordingPaused();

    /**
     * @brief Emitted when recording is resumed
     */
    void recordingResumed();

protected:
    // BaseService interface
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Handle media player state changes
     * @param state New playback state
     */
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

    /**
     * @brief Handle media player position changes
     * @param position New position in milliseconds
     */
    void onPositionChanged(qint64 position);

    /**
     * @brief Handle media player duration changes
     * @param duration New duration in milliseconds
     */
    void onDurationChanged(qint64 duration);

    /**
     * @brief Handle media player errors
     * @param error Error type
     * @param errorString Error description
     */
    void onMediaError(QMediaPlayer::Error error, const QString& errorString);

    /**
     * @brief Handle audio output volume changes
     * @param volume New volume level (0.0-1.0)
     */
    void onVolumeChanged(float volume);

    /**
     * @brief Handle audio devices list changes
     */
    void onAudioDevicesChanged();

    /**
     * @brief Handle media recorder state changes
     * @param state New recording state
     */
    void onRecordingStateChanged(QMediaRecorder::RecorderState state);

    /**
     * @brief Handle media recorder duration changes
     * @param duration New recording duration in milliseconds
     */
    void onRecordingDurationChanged(qint64 duration);

    /**
     * @brief Handle media recorder errors
     * @param error Error type
     * @param errorString Error description
     */
    void onRecordingError(QMediaRecorder::Error error, const QString& errorString);

private:
    /**
     * @brief Initialize the media player and audio output
     * @return true if initialization was successful
     */
    bool initializeMediaPlayer();

    /**
     * @brief Initialize audio device monitoring
     */
    void initializeDeviceMonitoring();

    /**
     * @brief Validate volume range
     * @param volume Volume to validate
     * @return Clamped volume value (0-100)
     */
    int validateVolume(int volume) const;

    /**
     * @brief Convert volume from 0-100 scale to 0.0-1.0 scale
     * @param volume Volume in 0-100 range
     * @return Volume in 0.0-1.0 range
     */
    float volumeToFloat(int volume) const;

    /**
     * @brief Convert volume from 0.0-1.0 scale to 0-100 scale
     * @param volume Volume in 0.0-1.0 range
     * @return Volume in 0-100 range
     */
    int volumeFromFloat(float volume) const;

    /**
     * @brief Check if the media player is in a valid state for operations
     * @return true if the player is ready for operations
     */
    bool isPlayerReady() const;

    /**
     * @brief Initialize the media recorder and audio input
     * @return true if initialization was successful
     */
    bool initializeMediaRecorder();

    /**
     * @brief Check if the media recorder is in a valid state for operations
     * @return true if the recorder is ready for operations
     */
    bool isRecorderReady() const;

    /**
     * @brief Validate recording output path
     * @param outputPath Path to validate
     * @return true if path is valid for recording
     */
    bool validateRecordingPath(const QString& outputPath) const;

    /**
     * @brief Setup recording format and quality settings
     * @return true if setup was successful
     */
    bool setupRecordingSettings();

    /**
     * @brief Get default recording format for the platform
     * @return Default media format for recording
     */
    QMediaFormat getDefaultRecordingFormat() const;

private:
    // Playback components
    std::unique_ptr<QMediaPlayer> m_mediaPlayer;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    QUrl m_currentMediaUrl;
    int m_currentVolume;
    
    // Recording components
    std::unique_ptr<QMediaRecorder> m_mediaRecorder;
    std::unique_ptr<QAudioInput> m_audioInput;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;
    QString m_currentRecordingPath;
    QMediaFormat m_recordingFormat;
    QMediaRecorder::Quality m_recordingQuality;
    int m_recordingSampleRate;
    int m_recordingBitRate;
    
    // Supported audio formats
    static const QStringList SUPPORTED_FORMATS;
    
    // Volume constraints
    static constexpr int MIN_VOLUME = 0;
    static constexpr int MAX_VOLUME = 100;
    static constexpr int DEFAULT_VOLUME = 75;
    
    // Recording constraints
    static constexpr int DEFAULT_SAMPLE_RATE = 44100;
    static constexpr int DEFAULT_BIT_RATE = 128000;
    static const QMediaRecorder::Quality DEFAULT_RECORDING_QUALITY;
};

#endif // AUDIOSERVICE_H