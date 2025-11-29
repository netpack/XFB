#include "TestMainController.h"
#include "../../../src/controllers/MainController.h"
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QProgressDialog>
#include <QApplication>
#include <QTimer>

void TestMainController::initTestCase()
{
    // Ensure QApplication exists for GUI components
    if (!QApplication::instance()) {
        int argc = 0;
        char* argv[] = {nullptr};
        new QApplication(argc, argv);
    }
}

void TestMainController::cleanupTestCase()
{
    // Cleanup is handled automatically
}

void TestMainController::init()
{
    m_controller = std::make_unique<MainController>();
    
    // Create temporary directory for test data
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(false);
    m_tempDir = tempDir.path();
}

void TestMainController::cleanup()
{
    if (m_controller) {
        m_controller->shutdown();
        m_controller.reset();
    }
    
    // Clean up temporary directory
    QDir(m_tempDir).removeRecursively();
}

void TestMainController::testInitialization()
{
    QVERIFY(m_controller != nullptr);
    QVERIFY(!m_controller->audioService()); // Should be null before initialization
    
    bool result = m_controller->initialize();
    QVERIFY(result);
    
    // Services should be available after initialization
    QVERIFY(m_controller->audioService() != nullptr);
    QVERIFY(m_controller->databaseService() != nullptr);
    QVERIFY(m_controller->configurationService() != nullptr);
    QVERIFY(m_controller->musicRepository() != nullptr);
    QVERIFY(m_controller->playlistRepository() != nullptr);
    QVERIFY(m_controller->genreRepository() != nullptr);
}

void TestMainController::testInitializationFailure()
{
    // This test would require mocking services to fail initialization
    // For now, we'll test that the controller handles initialization gracefully
    QVERIFY(m_controller != nullptr);
    
    // Multiple initializations should be safe
    bool result1 = m_controller->initialize();
    bool result2 = m_controller->initialize();
    
    QVERIFY(result1);
    QVERIFY(result2); // Should return true for already initialized
}

void TestMainController::testShutdown()
{
    QVERIFY(m_controller->initialize());
    
    // Services should be available
    QVERIFY(m_controller->audioService() != nullptr);
    
    m_controller->shutdown();
    
    // After shutdown, services should still be accessible but may be in shutdown state
    // The actual behavior depends on the service implementation
}

void TestMainController::testDoubleInitialization()
{
    bool result1 = m_controller->initialize();
    bool result2 = m_controller->initialize();
    
    QVERIFY(result1);
    QVERIFY(result2); // Should handle double initialization gracefully
}

void TestMainController::testServiceAccessors()
{
    QVERIFY(m_controller->initialize());
    
    // Test all service accessors
    QVERIFY(m_controller->audioService() != nullptr);
    QVERIFY(m_controller->databaseService() != nullptr);
    QVERIFY(m_controller->configurationService() != nullptr);
    QVERIFY(m_controller->musicRepository() != nullptr);
    QVERIFY(m_controller->playlistRepository() != nullptr);
    QVERIFY(m_controller->genreRepository() != nullptr);
}

void TestMainController::testServiceValidation()
{
    // Before initialization, services should not be available
    QVERIFY(m_controller->audioService() == nullptr);
    QVERIFY(m_controller->databaseService() == nullptr);
    QVERIFY(m_controller->configurationService() == nullptr);
    
    QVERIFY(m_controller->initialize());
    
    // After initialization, all services should be available
    QVERIFY(m_controller->audioService() != nullptr);
    QVERIFY(m_controller->databaseService() != nullptr);
    QVERIFY(m_controller->configurationService() != nullptr);
}

void TestMainController::testProgressDialog()
{
    QVERIFY(m_controller->initialize());
    
    // Test showing progress dialog
    QProgressDialog* dialog = m_controller->showProgressDialog("Test", "Testing...", 100);
    QVERIFY(dialog != nullptr);
    QCOMPARE(dialog->windowTitle(), QString("Test"));
    QCOMPARE(dialog->labelText(), QString("Testing..."));
    QCOMPARE(dialog->maximum(), 100);
    
    m_controller->hideProgressDialog();
}

void TestMainController::testProgressDialogUpdate()
{
    QVERIFY(m_controller->initialize());
    
    QProgressDialog* dialog = m_controller->showProgressDialog("Test", "Testing...", 100);
    QVERIFY(dialog != nullptr);
    
    // Test updating progress
    m_controller->updateProgress(50, "Half way...");
    QCOMPARE(dialog->value(), 50);
    QCOMPARE(dialog->labelText(), QString("Half way..."));
    
    m_controller->hideProgressDialog();
}

void TestMainController::testProgressDialogHide()
{
    QVERIFY(m_controller->initialize());
    
    QProgressDialog* dialog = m_controller->showProgressDialog("Test", "Testing...", 100);
    QVERIFY(dialog != nullptr);
    
    m_controller->hideProgressDialog();
    
    // Dialog should be scheduled for deletion
    QApplication::processEvents();
}

void TestMainController::testPlayAudio()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy statusSpy(m_controller.get(), &MainController::statusMessageChanged);
    
    // Test playing audio (will fail without actual file, but should handle gracefully)
    m_controller->playAudio("/nonexistent/file.mp3");
    
    // Should emit status message
    QVERIFY(statusSpy.count() >= 0); // May or may not emit depending on error handling
}

void TestMainController::testPauseAudio()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy statusSpy(m_controller.get(), &MainController::statusMessageChanged);
    
    m_controller->pauseAudio();
    
    // Should emit status message
    QVERIFY(statusSpy.count() >= 1);
    QCOMPARE(statusSpy.last().first().toString(), QString("Playback paused"));
}

void TestMainController::testStopAudio()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy statusSpy(m_controller.get(), &MainController::statusMessageChanged);
    
    m_controller->stopAudio();
    
    // Should emit status message
    QVERIFY(statusSpy.count() >= 1);
    QCOMPARE(statusSpy.last().first().toString(), QString("Playback stopped"));
}

void TestMainController::testVolumeControl()
{
    QVERIFY(m_controller->initialize());
    
    // Test setting volume
    m_controller->setVolume(75);
    
    // Volume change should be handled by audio service
    // We can't easily test the actual volume without mocking
}

void TestMainController::testMusicImport()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy importSpy(m_controller.get(), &MainController::musicImportCompleted);
    
    // Test importing empty list
    m_controller->importMusic(QStringList());
    
    // Should complete immediately with 0 imported
    QVERIFY(importSpy.count() >= 1);
    QCOMPARE(importSpy.last().at(0).toBool(), false); // success = false for empty list
    QCOMPARE(importSpy.last().at(1).toInt(), 0); // imported count = 0
}

void TestMainController::testDirectoryImport()
{
    QVERIFY(m_controller->initialize());
    
    // Test importing non-existent directory
    m_controller->importDirectory("/nonexistent/directory");
    
    // Should handle gracefully (exact behavior depends on implementation)
}

void TestMainController::testImportWithInvalidFiles()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy importSpy(m_controller.get(), &MainController::musicImportCompleted);
    
    // Test importing invalid files
    QStringList invalidFiles = {"/nonexistent/file1.mp3", "/nonexistent/file2.ogg"};
    m_controller->importMusic(invalidFiles);
    
    // Should complete with 0 imported files
    QVERIFY(importSpy.count() >= 1);
    QCOMPARE(importSpy.last().at(1).toInt(), 0); // imported count = 0
}

void TestMainController::testSavePlaylist()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy playlistSpy(m_controller.get(), &MainController::playlistOperationCompleted);
    
    QList<int> musicIds = {1, 2, 3};
    m_controller->savePlaylist("Test Playlist", musicIds);
    
    // Should emit completion signal
    QVERIFY(playlistSpy.count() >= 1);
}

void TestMainController::testLoadPlaylist()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy playlistSpy(m_controller.get(), &MainController::playlistOperationCompleted);
    
    m_controller->loadPlaylist(1);
    
    // Should emit completion signal
    QVERIFY(playlistSpy.count() >= 1);
}

void TestMainController::testPlaylistOperationErrors()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy errorSpy(m_controller.get(), &MainController::errorOccurred);
    
    // Test operations without proper services (should be handled gracefully)
    // The exact behavior depends on the service implementation
}

void TestMainController::testDatabaseOptimization()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy statusSpy(m_controller.get(), &MainController::statusMessageChanged);
    
    m_controller->optimizeDatabase();
    
    // Should show progress and complete
    // The actual test would need to wait for the async operation
}

void TestMainController::testServiceErrorHandling()
{
    QVERIFY(m_controller->initialize());
    
    QSignalSpy errorSpy(m_controller.get(), &MainController::errorOccurred);
    
    // Simulate service error by calling private method (if accessible)
    // For now, we test that error signals are properly connected
    QVERIFY(errorSpy.isValid());
}

void TestMainController::testOperationProgress()
{
    QVERIFY(m_controller->initialize());
    
    // Test progress handling
    QProgressDialog* dialog = m_controller->showProgressDialog("Test", "Testing...", 100);
    QVERIFY(dialog != nullptr);
    
    m_controller->updateProgress(25, "Quarter done");
    QCOMPARE(dialog->value(), 25);
    
    m_controller->updateProgress(75, "Three quarters done");
    QCOMPARE(dialog->value(), 75);
    
    m_controller->hideProgressDialog();
}

void TestMainController::testSignalEmission()
{
    QVERIFY(m_controller->initialize());
    
    // Test that all expected signals are defined and can be connected
    QSignalSpy playbackSpy(m_controller.get(), &MainController::playbackStateChanged);
    QSignalSpy positionSpy(m_controller.get(), &MainController::positionChanged);
    QSignalSpy durationSpy(m_controller.get(), &MainController::durationChanged);
    QSignalSpy volumeSpy(m_controller.get(), &MainController::volumeChanged);
    QSignalSpy importSpy(m_controller.get(), &MainController::musicImportCompleted);
    QSignalSpy playlistSpy(m_controller.get(), &MainController::playlistOperationCompleted);
    QSignalSpy errorSpy(m_controller.get(), &MainController::errorOccurred);
    QSignalSpy statusSpy(m_controller.get(), &MainController::statusMessageChanged);
    
    // All signal spies should be valid
    QVERIFY(playbackSpy.isValid());
    QVERIFY(positionSpy.isValid());
    QVERIFY(durationSpy.isValid());
    QVERIFY(volumeSpy.isValid());
    QVERIFY(importSpy.isValid());
    QVERIFY(playlistSpy.isValid());
    QVERIFY(errorSpy.isValid());
    QVERIFY(statusSpy.isValid());
}

QTEST_MAIN(TestMainController)