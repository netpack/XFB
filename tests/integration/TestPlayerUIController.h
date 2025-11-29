#ifndef TESTPLAYERUICONTROLLER_H
#define TESTPLAYERUICONTROLLER_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QTemporaryDir>
#include <memory>

// Forward declarations
class PlayerUIController;
class player;
class MainController;
class QApplication;

/**
 * @brief Integration tests for PlayerUIController
 * 
 * These tests verify the integration between the UI controller and the main controller,
 * ensuring proper signal-slot connections and UI responsiveness.
 * 
 * @since XFB 2.0
 */
class TestPlayerUIController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Controller initialization tests
    void testControllerInitialization();
    void testControllerShutdown();
    void testMainControllerAccess();

    // UI signal connection tests
    void testPlayButtonConnection();
    void testVolumeSliderConnection();
    void testProgressSliderConnection();
    void testMusicSelectionConnection();

    // Controller signal handling tests
    void testPlaybackStateUpdates();
    void testPositionUpdates();
    void testDurationUpdates();
    void testVolumeUpdates();
    void testErrorHandling();
    void testStatusMessageUpdates();

    // Long-running operation tests
    void testMusicImportProgress();
    void testDirectoryImportProgress();
    void testDatabaseOptimizationProgress();
    void testProgressDialogCancellation();

    // UI responsiveness tests
    void testUIResponsivenessDuringImport();
    void testUIRefreshTimer();
    void testProgressIndicatorUpdates();

    // Integration workflow tests
    void testCompletePlaybackWorkflow();
    void testCompleteImportWorkflow();
    void testCompletePlaylistWorkflow();

    // Error scenario tests
    void testControllerInitializationFailure();
    void testServiceUnavailableHandling();
    void testInvalidFileHandling();

private:
    /**
     * @brief Create a test player window
     * @return Pointer to the created player window
     */
    player* createTestPlayerWindow();

    /**
     * @brief Create test audio files for testing
     * @return List of created test file paths
     */
    QStringList createTestAudioFiles();

    /**
     * @brief Wait for signal with timeout
     * @param spy Signal spy to wait for
     * @param timeout Timeout in milliseconds
     * @return true if signal was received
     */
    bool waitForSignal(QSignalSpy* spy, int timeout = 5000);

    /**
     * @brief Simulate user interaction with UI element
     * @param widget Widget to interact with
     * @param action Action to perform
     */
    void simulateUserInteraction(QWidget* widget, const QString& action);

    /**
     * @brief Verify UI state matches expected values
     * @param controller Controller to check
     * @param expectedState Expected state values
     * @return true if state matches
     */
    bool verifyUIState(PlayerUIController* controller, const QVariantMap& expectedState);

    /**
     * @brief Create a mock long-running operation
     * @param duration Duration in milliseconds
     */
    void simulateLongRunningOperation(int duration);

private:
    std::unique_ptr<QApplication> m_app;
    std::unique_ptr<player> m_playerWindow;
    std::unique_ptr<PlayerUIController> m_controller;
    QTemporaryDir m_tempDir;
    QStringList m_testFiles;
    QTimer* m_testTimer;
};

#endif // TESTPLAYERUICONTROLLER_H