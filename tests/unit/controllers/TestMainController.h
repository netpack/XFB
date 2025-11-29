#ifndef TESTMAINCONTROLLER_H
#define TESTMAINCONTROLLER_H

#include <QtTest/QtTest>
#include <QObject>
#include <memory>

class MainController;

/**
 * @brief Unit tests for MainController
 * 
 * Tests the controller pattern implementation, service coordination,
 * and UI interaction handling.
 */
class TestMainController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Initialization tests
    void testInitialization();
    void testInitializationFailure();
    void testShutdown();
    void testDoubleInitialization();

    // Service access tests
    void testServiceAccessors();
    void testServiceValidation();

    // Progress dialog tests
    void testProgressDialog();
    void testProgressDialogUpdate();
    void testProgressDialogHide();

    // Audio control tests
    void testPlayAudio();
    void testPauseAudio();
    void testStopAudio();
    void testVolumeControl();

    // Music import tests
    void testMusicImport();
    void testDirectoryImport();
    void testImportWithInvalidFiles();

    // Playlist operations tests
    void testSavePlaylist();
    void testLoadPlaylist();
    void testPlaylistOperationErrors();

    // Database operations tests
    void testDatabaseOptimization();

    // Error handling tests
    void testServiceErrorHandling();
    void testOperationProgress();

    // Signal emission tests
    void testSignalEmission();

private:
    std::unique_ptr<MainController> m_controller;
    QString m_tempDir;
};

#endif // TESTMAINCONTROLLER_H