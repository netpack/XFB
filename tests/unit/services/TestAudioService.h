#ifndef TESTAUDIOSERVICE_H
#define TESTAUDIOSERVICE_H

#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <memory>

class AudioService;

/**
 * @brief Test class for AudioService recording functionality
 * 
 * This test class focuses on testing the recording capabilities of AudioService,
 * including format selection, quality settings, error handling, and device management.
 */
class TestAudioService : public QObject
{
    Q_OBJECT

private slots:
    // Test setup and cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Recording functionality tests
    void testRecordingInitialization();
    void testStartRecording();
    void testStopRecording();
    void testPauseResumeRecording();
    void testRecordingToInvalidPath();
    void testRecordingToReadOnlyDirectory();
    void testMultipleRecordingAttempts();
    
    // Recording settings tests
    void testRecordingQualitySettings();
    void testRecordingFormatSettings();
    void testRecordingSampleRateSettings();
    void testRecordingBitRateSettings();
    void testUnsupportedFormatHandling();
    
    // Device management tests
    void testInputDeviceEnumeration();
    void testInputDeviceSelection();
    void testInvalidInputDeviceHandling();
    void testDeviceChangeHandling();
    
    // Error handling tests
    void testRecordingErrorHandling();
    void testRecordingWithoutInitialization();
    void testRecordingStateTransitions();
    void testRecordingDurationTracking();
    
    // Signal emission tests
    void testRecordingSignals();
    void testRecordingErrorSignals();
    void testRecordingStateSignals();
    void testRecordingDurationSignals();
    
    // Integration tests
    void testRecordingWithPlayback();
    void testRecordingFileCreation();
    void testRecordingFileFormat();

private:
    // Helper methods
    void setupValidRecordingPath();
    void createReadOnlyDirectory();
    void waitForRecordingState(QMediaRecorder::RecorderState expectedState, int timeoutMs = 5000);
    bool isRecordingSupported() const;
    QString getValidRecordingPath(const QString& filename) const;
    void verifyRecordingFile(const QString& filePath) const;

private:
    std::unique_ptr<AudioService> m_audioService;
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_validRecordingPath;
    QString m_readOnlyPath;
};

#endif // TESTAUDIOSERVICE_H