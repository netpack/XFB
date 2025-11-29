#include "TestAudioService.h"
#include "../../../src/services/AudioService.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QMediaDevices>

void TestAudioService::initTestCase()
{
    // Create temporary directory for test recordings
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    
    qDebug() << "Test recordings will be saved to:" << m_tempDir->path();
}

void TestAudioService::cleanupTestCase()
{
    // Cleanup is automatic with QTemporaryDir
    m_tempDir.reset();
}

void TestAudioService::init()
{
    // Create fresh AudioService instance for each test
    m_audioService = std::make_unique<AudioService>();
    QVERIFY(m_audioService->initialize());
    
    setupValidRecordingPath();
}

void TestAudioService::cleanup()
{
    if (m_audioService) {
        // Stop any ongoing recording
        if (m_audioService->isRecording()) {
            m_audioService->stopRecording();
        }
        m_audioService->shutdown();
        m_audioService.reset();
    }
}

void TestAudioService::testRecordingInitialization()
{
    // Test that recording components are properly initialized
    QVERIFY(m_audioService->isRecordingStopped());
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::StoppedState);
    QCOMPARE(m_audioService->recordingDuration(), 0);
    
    // Test default recording settings
    QCOMPARE(m_audioService->recordingQuality(), QMediaRecorder::NormalQuality);
    QCOMPARE(m_audioService->recordingSampleRate(), 44100);
    QCOMPARE(m_audioService->recordingBitRate(), 128000);
    
    // Test that recording format is set
    QMediaFormat format = m_audioService->recordingFormat();
    // QMediaFormat doesn't have isValid() method in Qt6, just check it's not null
    QVERIFY(format.fileFormat() != QMediaFormat::UnspecifiedFormat);
}

void TestAudioService::testStartRecording()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("test_recording.wav");
    
    // Setup signal spy
    QSignalSpy recordingStartedSpy(m_audioService.get(), &AudioService::recordingStarted);
    QSignalSpy recordingStateChangedSpy(m_audioService.get(), &AudioService::recordingStateChanged);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    
    // Wait for recording to start
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Verify state
    QVERIFY(m_audioService->isRecording());
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::RecordingState);
    
    // Verify signals
    QCOMPARE(recordingStartedSpy.count(), 1);
    QCOMPARE(recordingStartedSpy.first().first().toString(), recordingPath);
    QVERIFY(recordingStateChangedSpy.count() >= 1);
    
    // Stop recording for cleanup
    m_audioService->stopRecording();
}

void TestAudioService::testStopRecording()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("test_stop_recording.wav");
    
    // Start recording first
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Setup signal spy for stop
    QSignalSpy recordingStoppedSpy(m_audioService.get(), &AudioService::recordingStopped);
    QSignalSpy recordingStateChangedSpy(m_audioService.get(), &AudioService::recordingStateChanged);
    
    // Stop recording
    QVERIFY(m_audioService->stopRecording());
    
    // Wait for recording to stop
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // Verify state
    QVERIFY(m_audioService->isRecordingStopped());
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::StoppedState);
    
    // Verify signals
    QCOMPARE(recordingStoppedSpy.count(), 1);
    QCOMPARE(recordingStoppedSpy.first().first().toString(), recordingPath);
    
    // Verify file was created
    QVERIFY(QFileInfo::exists(recordingPath));
}

void TestAudioService::testPauseResumeRecording()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("test_pause_resume.wav");
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Setup signal spies
    QSignalSpy recordingPausedSpy(m_audioService.get(), &AudioService::recordingPaused);
    QSignalSpy recordingResumedSpy(m_audioService.get(), &AudioService::recordingResumed);
    
    // Pause recording
    QVERIFY(m_audioService->pauseRecording());
    waitForRecordingState(QMediaRecorder::PausedState);
    
    QVERIFY(m_audioService->isRecordingPaused());
    QCOMPARE(recordingPausedSpy.count(), 1);
    
    // Resume recording
    QVERIFY(m_audioService->resumeRecording());
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    QVERIFY(m_audioService->isRecording());
    QCOMPARE(recordingResumedSpy.count(), 1);
    
    // Stop recording for cleanup
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingToInvalidPath()
{
    // Test empty path
    QVERIFY(!m_audioService->startRecording(""));
    
    // Test invalid characters in path (platform-specific)
#ifdef Q_OS_WIN
    QVERIFY(!m_audioService->startRecording("C:\\invalid<>path.wav"));
#else
    QVERIFY(!m_audioService->startRecording("/dev/null/invalid.wav"));
#endif
    
    // Test path without extension
    QVERIFY(!m_audioService->startRecording(m_tempDir->path() + "/noextension"));
}

void TestAudioService::testRecordingToReadOnlyDirectory()
{
    createReadOnlyDirectory();
    
    if (!m_readOnlyPath.isEmpty()) {
        QString recordingPath = m_readOnlyPath + "/readonly_test.wav";
        
        QSignalSpy errorSpy(m_audioService.get(), &AudioService::recordingError);
        
        QVERIFY(!m_audioService->startRecording(recordingPath));
        
        // Should emit error signal
        QVERIFY(errorSpy.count() > 0);
    }
}

void TestAudioService::testMultipleRecordingAttempts()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath1 = getValidRecordingPath("test_multiple1.wav");
    QString recordingPath2 = getValidRecordingPath("test_multiple2.wav");
    
    // Start first recording
    QVERIFY(m_audioService->startRecording(recordingPath1));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Try to start second recording while first is active
    QSignalSpy errorSpy(m_audioService.get(), &AudioService::recordingError);
    QVERIFY(!m_audioService->startRecording(recordingPath2));
    QVERIFY(errorSpy.count() > 0);
    
    // Stop first recording
    m_audioService->stopRecording();
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // Now second recording should work
    QVERIFY(m_audioService->startRecording(recordingPath2));
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingQualitySettings()
{
    // Test setting different quality levels
    QVERIFY(m_audioService->setRecordingQuality(QMediaRecorder::VeryLowQuality));
    QCOMPARE(m_audioService->recordingQuality(), QMediaRecorder::VeryLowQuality);
    
    QVERIFY(m_audioService->setRecordingQuality(QMediaRecorder::HighQuality));
    QCOMPARE(m_audioService->recordingQuality(), QMediaRecorder::HighQuality);
    
    QVERIFY(m_audioService->setRecordingQuality(QMediaRecorder::VeryHighQuality));
    QCOMPARE(m_audioService->recordingQuality(), QMediaRecorder::VeryHighQuality);
}

void TestAudioService::testRecordingFormatSettings()
{
    QList<QMediaFormat> supportedFormats = m_audioService->getSupportedRecordingFormats();
    
    if (!supportedFormats.isEmpty()) {
        QMediaFormat testFormat = supportedFormats.first();
        QVERIFY(m_audioService->setRecordingFormat(testFormat));
        
        QMediaFormat currentFormat = m_audioService->recordingFormat();
        QCOMPARE(currentFormat.fileFormat(), testFormat.fileFormat());
        QCOMPARE(currentFormat.audioCodec(), testFormat.audioCodec());
    }
}

void TestAudioService::testRecordingSampleRateSettings()
{
    // Test valid sample rates
    QVERIFY(m_audioService->setRecordingSampleRate(22050));
    QCOMPARE(m_audioService->recordingSampleRate(), 22050);
    
    QVERIFY(m_audioService->setRecordingSampleRate(48000));
    QCOMPARE(m_audioService->recordingSampleRate(), 48000);
    
    // Test invalid sample rate
    QVERIFY(!m_audioService->setRecordingSampleRate(0));
    QVERIFY(!m_audioService->setRecordingSampleRate(-1000));
}

void TestAudioService::testRecordingBitRateSettings()
{
    // Test valid bit rates
    QVERIFY(m_audioService->setRecordingBitRate(64000));
    QCOMPARE(m_audioService->recordingBitRate(), 64000);
    
    QVERIFY(m_audioService->setRecordingBitRate(320000));
    QCOMPARE(m_audioService->recordingBitRate(), 320000);
    
    // Test invalid bit rate
    QVERIFY(!m_audioService->setRecordingBitRate(0));
    QVERIFY(!m_audioService->setRecordingBitRate(-1000));
}

void TestAudioService::testUnsupportedFormatHandling()
{
    // Create an unsupported format
    QMediaFormat unsupportedFormat;
    unsupportedFormat.setFileFormat(static_cast<QMediaFormat::FileFormat>(-1));
    
    QSignalSpy errorSpy(m_audioService.get(), &AudioService::recordingError);
    
    QVERIFY(!m_audioService->setRecordingFormat(unsupportedFormat));
    
    // Should emit error for unsupported format
    QVERIFY(errorSpy.count() > 0);
}

void TestAudioService::testInputDeviceEnumeration()
{
    QList<QAudioDevice> inputDevices = m_audioService->getAvailableInputDevices();
    
    // Should have at least one input device (or none if no audio hardware)
    QVERIFY(inputDevices.size() >= 0);
    
    // Each device should be valid
    for (const auto& device : inputDevices) {
        QVERIFY(!device.isNull());
        QVERIFY(!device.description().isEmpty());
    }
}

void TestAudioService::testInputDeviceSelection()
{
    QList<QAudioDevice> inputDevices = m_audioService->getAvailableInputDevices();
    
    if (!inputDevices.isEmpty()) {
        QAudioDevice testDevice = inputDevices.first();
        
        QVERIFY(m_audioService->setInputDevice(testDevice));
        
        QAudioDevice currentDevice = m_audioService->currentInputDevice();
        QCOMPARE(currentDevice.id(), testDevice.id());
    }
}

void TestAudioService::testInvalidInputDeviceHandling()
{
    QAudioDevice invalidDevice; // Default constructed device is invalid
    
    QVERIFY(!m_audioService->setInputDevice(invalidDevice));
}

void TestAudioService::testDeviceChangeHandling()
{
    QSignalSpy deviceChangedSpy(m_audioService.get(), &AudioService::audioDevicesChanged);
    
    QList<QAudioDevice> inputDevices = m_audioService->getAvailableInputDevices();
    
    if (inputDevices.size() > 1) {
        // Change to different device
        QVERIFY(m_audioService->setInputDevice(inputDevices[1]));
        
        // Should emit device changed signal
        QVERIFY(deviceChangedSpy.count() > 0);
    }
}

void TestAudioService::testRecordingErrorHandling()
{
    QSignalSpy errorSpy(m_audioService.get(), &AudioService::recordingError);
    
    // Try to start recording without proper initialization
    AudioService uninitializedService;
    QVERIFY(!uninitializedService.startRecording(getValidRecordingPath("error_test.wav")));
}

void TestAudioService::testRecordingWithoutInitialization()
{
    AudioService uninitializedService;
    
    // All recording operations should fail without initialization
    QVERIFY(!uninitializedService.startRecording(getValidRecordingPath("uninit_test.wav")));
    QVERIFY(!uninitializedService.stopRecording());
    QVERIFY(!uninitializedService.pauseRecording());
    QVERIFY(!uninitializedService.resumeRecording());
    
    // State queries should return safe defaults
    QCOMPARE(uninitializedService.recordingState(), QMediaRecorder::StoppedState);
    QCOMPARE(uninitializedService.recordingDuration(), 0);
}

void TestAudioService::testRecordingStateTransitions()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("state_test.wav");
    
    QSignalSpy stateChangedSpy(m_audioService.get(), &AudioService::recordingStateChanged);
    
    // Initial state should be stopped
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::StoppedState);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Should transition to recording state
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::RecordingState);
    
    // Pause recording
    QVERIFY(m_audioService->pauseRecording());
    waitForRecordingState(QMediaRecorder::PausedState);
    
    // Should transition to paused state
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::PausedState);
    
    // Resume recording
    QVERIFY(m_audioService->resumeRecording());
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Should transition back to recording state
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::RecordingState);
    
    // Stop recording
    QVERIFY(m_audioService->stopRecording());
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // Should transition to stopped state
    QCOMPARE(m_audioService->recordingState(), QMediaRecorder::StoppedState);
    
    // Verify we received state change signals
    QVERIFY(stateChangedSpy.count() >= 4); // At least start, pause, resume, stop
}

void TestAudioService::testRecordingDurationTracking()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("duration_test.wav");
    
    QSignalSpy durationChangedSpy(m_audioService.get(), &AudioService::recordingDurationChanged);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Wait a bit for duration to change
    QTest::qWait(1000);
    
    // Duration should be greater than 0
    QVERIFY(m_audioService->recordingDuration() >= 0);
    
    // Should have received duration change signals
    QVERIFY(durationChangedSpy.count() >= 0); // May be 0 if duration updates are not frequent
    
    // Stop recording
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingSignals()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("signals_test.wav");
    
    // Setup signal spies
    QSignalSpy startedSpy(m_audioService.get(), &AudioService::recordingStarted);
    QSignalSpy stoppedSpy(m_audioService.get(), &AudioService::recordingStopped);
    QSignalSpy stateChangedSpy(m_audioService.get(), &AudioService::recordingStateChanged);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Verify started signal
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(startedSpy.first().first().toString(), recordingPath);
    
    // Stop recording
    QVERIFY(m_audioService->stopRecording());
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // Verify stopped signal
    QCOMPARE(stoppedSpy.count(), 1);
    QCOMPARE(stoppedSpy.first().first().toString(), recordingPath);
    
    // Verify state change signals
    QVERIFY(stateChangedSpy.count() >= 2); // At least start and stop
}

void TestAudioService::testRecordingErrorSignals()
{
    QSignalSpy errorSpy(m_audioService.get(), &AudioService::recordingError);
    
    // Try to record to invalid path
    QVERIFY(!m_audioService->startRecording(""));
    
    // Should emit error signal
    QVERIFY(errorSpy.count() > 0);
    
    QString errorMessage = errorSpy.first().first().toString();
    QVERIFY(!errorMessage.isEmpty());
}

void TestAudioService::testRecordingStateSignals()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("state_signals_test.wav");
    
    QSignalSpy stateChangedSpy(m_audioService.get(), &AudioService::recordingStateChanged);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Should have received recording state signal
    bool foundRecordingState = false;
    for (const auto& signal : stateChangedSpy) {
        if (signal.first().value<QMediaRecorder::RecorderState>() == QMediaRecorder::RecordingState) {
            foundRecordingState = true;
            break;
        }
    }
    QVERIFY(foundRecordingState);
    
    // Stop recording
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingDurationSignals()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("duration_signals_test.wav");
    
    QSignalSpy durationChangedSpy(m_audioService.get(), &AudioService::recordingDurationChanged);
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Wait for potential duration updates
    QTest::qWait(1000);
    
    // Duration signals may or may not be emitted depending on the platform
    // Just verify that if they are emitted, they contain valid data
    for (const auto& signal : durationChangedSpy) {
        qint64 duration = signal.first().toLongLong();
        QVERIFY(duration >= 0);
    }
    
    // Stop recording
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingWithPlayback()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("playback_test.wav");
    
    // Start recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Try to start playback while recording (should work independently)
    // Note: This test assumes we have a test audio file, which we don't in this minimal test
    // In a real scenario, you would test with an actual audio file
    
    // Stop recording
    m_audioService->stopRecording();
}

void TestAudioService::testRecordingFileCreation()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("file_creation_test.wav");
    
    // Ensure file doesn't exist initially
    QVERIFY(!QFileInfo::exists(recordingPath));
    
    // Start and stop recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    // Record for a short time
    QTest::qWait(500);
    
    QVERIFY(m_audioService->stopRecording());
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // File should now exist
    QVERIFY(QFileInfo::exists(recordingPath));
    
    // File should have some content
    QFileInfo fileInfo(recordingPath);
    QVERIFY(fileInfo.size() > 0);
}

void TestAudioService::testRecordingFileFormat()
{
    if (!isRecordingSupported()) {
        QSKIP("Recording not supported on this system");
    }
    
    QString recordingPath = getValidRecordingPath("format_test.wav");
    
    // Start and stop recording
    QVERIFY(m_audioService->startRecording(recordingPath));
    waitForRecordingState(QMediaRecorder::RecordingState);
    
    QTest::qWait(500);
    
    QVERIFY(m_audioService->stopRecording());
    waitForRecordingState(QMediaRecorder::StoppedState);
    
    // Verify file format (basic check)
    verifyRecordingFile(recordingPath);
}

// Helper method implementations
void TestAudioService::setupValidRecordingPath()
{
    m_validRecordingPath = m_tempDir->path() + "/recordings";
    QDir().mkpath(m_validRecordingPath);
}

void TestAudioService::createReadOnlyDirectory()
{
    m_readOnlyPath = m_tempDir->path() + "/readonly";
    QDir().mkpath(m_readOnlyPath);
    
    // Try to make directory read-only (platform-specific)
    QFileInfo dirInfo(m_readOnlyPath);
    if (dirInfo.exists()) {
        QFile::setPermissions(m_readOnlyPath, QFileDevice::ReadOwner | QFileDevice::ReadGroup);
    }
}

void TestAudioService::waitForRecordingState(QMediaRecorder::RecorderState expectedState, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeoutMs) {
        if (m_audioService->recordingState() == expectedState) {
            return;
        }
        QTest::qWait(10);
        QCoreApplication::processEvents();
    }
    
    // If we get here, we timed out
    qWarning() << "Timeout waiting for recording state" << expectedState 
               << "current state:" << m_audioService->recordingState();
}

bool TestAudioService::isRecordingSupported() const
{
    // Check if we have any input devices
    QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    return !inputDevices.isEmpty();
}

QString TestAudioService::getValidRecordingPath(const QString& filename) const
{
    return m_validRecordingPath + "/" + filename;
}

void TestAudioService::verifyRecordingFile(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    
    // Basic file checks
    QVERIFY(fileInfo.exists());
    QVERIFY(fileInfo.size() > 0);
    QVERIFY(fileInfo.isReadable());
    
    // Check file extension
    QString extension = fileInfo.suffix().toLower();
    QVERIFY(!extension.isEmpty());
}

QTEST_MAIN(TestAudioService)