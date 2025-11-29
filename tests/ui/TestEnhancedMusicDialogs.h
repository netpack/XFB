#ifndef TESTENHANCEDMUSICDIALOGS_H
#define TESTENHANCEDMUSICDIALOGS_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <memory>

// Forward declarations
class EnhancedAddMusicSingleDialog;
class EnhancedAddDirectoryDialog;
class MusicRepository;
class QSqlDatabase;
class QApplication;

/**
 * @brief UI tests for enhanced music management dialogs
 * 
 * These tests verify the functionality, validation, and user experience
 * of the enhanced music management dialogs.
 * 
 * @since XFB 2.0
 */
class TestEnhancedMusicDialogs : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // EnhancedAddMusicSingleDialog tests
    void testSingleDialogInitialization();
    void testSingleDialogFileSelection();
    void testSingleDialogValidation();
    void testSingleDialogMetadataExtraction();
    void testSingleDialogFormValidation();
    void testSingleDialogErrorHandling();
    void testSingleDialogProgressIndication();
    void testSingleDialogMusicAddition();

    // EnhancedAddDirectoryDialog tests
    void testDirectoryDialogInitialization();
    void testDirectoryDialogDirectorySelection();
    void testDirectoryDialogFileScanning();
    void testDirectoryDialogFileFiltering();
    void testDirectoryDialogBatchImport();
    void testDirectoryDialogProgressTracking();
    void testDirectoryDialogCancellation();
    void testDirectoryDialogErrorHandling();

    // Validation tests
    void testInputValidation_data();
    void testInputValidation();
    void testFilePathValidation_data();
    void testFilePathValidation();
    void testRequiredFieldValidation();
    void testRealTimeValidation();

    // User interaction tests
    void testKeyboardNavigation();
    void testTabOrder();
    void testAccessibilityFeatures();
    void testTooltipsAndHelpText();

    // Integration tests
    void testDialogIntegrationWithRepository();
    void testDialogIntegrationWithServices();
    void testDialogChaining();

    // Performance tests
    void testLargeDirectoryHandling();
    void testMetadataExtractionPerformance();
    void testUIResponsiveness();

    // Error scenario tests
    void testInvalidFileHandling();
    void testDatabaseErrorHandling();
    void testNetworkErrorHandling();
    void testPermissionErrorHandling();

private:
    /**
     * @brief Create test database with sample data
     * @return true if database was created successfully
     */
    bool createTestDatabase();

    /**
     * @brief Create test audio files
     * @return List of created test file paths
     */
    QStringList createTestAudioFiles();

    /**
     * @brief Create test directory structure
     * @return Path to the test directory
     */
    QString createTestDirectoryStructure();

    /**
     * @brief Simulate user input in dialog
     * @param dialog Dialog to interact with
     * @param inputs Map of field names to values
     */
    void simulateUserInput(QDialog* dialog, const QVariantMap& inputs);

    /**
     * @brief Verify dialog state
     * @param dialog Dialog to verify
     * @param expectedState Expected state values
     * @return true if state matches expectations
     */
    bool verifyDialogState(QDialog* dialog, const QVariantMap& expectedState);

    /**
     * @brief Wait for dialog operation to complete
     * @param dialog Dialog to wait for
     * @param timeoutMs Timeout in milliseconds
     * @return true if operation completed within timeout
     */
    bool waitForDialogOperation(QDialog* dialog, int timeoutMs = 5000);

    /**
     * @brief Simulate file selection in dialog
     * @param dialog Dialog to interact with
     * @param filePath Path to select
     */
    void simulateFileSelection(QDialog* dialog, const QString& filePath);

    /**
     * @brief Simulate directory selection in dialog
     * @param dialog Dialog to interact with
     * @param directoryPath Directory to select
     */
    void simulateDirectorySelection(QDialog* dialog, const QString& directoryPath);

    /**
     * @brief Test validation for a specific field
     * @param dialog Dialog containing the field
     * @param fieldName Name of the field to test
     * @param validValues List of valid values
     * @param invalidValues List of invalid values
     */
    void testFieldValidation(QDialog* dialog, 
                           const QString& fieldName,
                           const QStringList& validValues,
                           const QStringList& invalidValues);

    /**
     * @brief Verify error message display
     * @param dialog Dialog to check
     * @param expectedError Expected error message
     * @return true if error is displayed correctly
     */
    bool verifyErrorDisplay(QDialog* dialog, const QString& expectedError);

    /**
     * @brief Verify progress indication
     * @param dialog Dialog to check
     * @param expectedProgress Expected progress state
     * @return true if progress is displayed correctly
     */
    bool verifyProgressIndication(QDialog* dialog, const QVariantMap& expectedProgress);

    /**
     * @brief Create mock metadata for testing
     * @param filePath File path for metadata
     * @return Mock metadata map
     */
    QVariantMap createMockMetadata(const QString& filePath);

    /**
     * @brief Simulate long-running operation
     * @param durationMs Duration in milliseconds
     */
    void simulateLongOperation(int durationMs);

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<QSqlDatabase> m_database;
    std::unique_ptr<MusicRepository> m_repository;
    std::unique_ptr<EnhancedAddMusicSingleDialog> m_singleDialog;
    std::unique_ptr<EnhancedAddDirectoryDialog> m_directoryDialog;
    QStringList m_testFiles;
    QString m_testDirectory;
};

/**
 * @brief Mock metadata extractor for testing
 */
class MockMetadataExtractor : public QObject
{
    Q_OBJECT

public:
    explicit MockMetadataExtractor(QObject* parent = nullptr);
    
    /**
     * @brief Set mock metadata for a file
     * @param filePath File path
     * @param metadata Mock metadata
     */
    void setMockMetadata(const QString& filePath, const QVariantMap& metadata);
    
    /**
     * @brief Extract mock metadata
     * @param filePath File path
     * @return Mock metadata
     */
    QVariantMap extractMetadata(const QString& filePath);
    
    /**
     * @brief Simulate extraction delay
     * @param delayMs Delay in milliseconds
     */
    void setExtractionDelay(int delayMs);

private:
    QMap<QString, QVariantMap> m_mockMetadata;
    int m_extractionDelay;
};

/**
 * @brief Dialog test helper class
 */
class DialogTestHelper
{
public:
    /**
     * @brief Find widget by name in dialog
     * @param dialog Dialog to search in
     * @param widgetName Name of the widget
     * @return Pointer to the widget (nullptr if not found)
     */
    static QWidget* findWidget(QDialog* dialog, const QString& widgetName);
    
    /**
     * @brief Set widget value
     * @param widget Widget to set value for
     * @param value Value to set
     * @return true if value was set successfully
     */
    static bool setWidgetValue(QWidget* widget, const QVariant& value);
    
    /**
     * @brief Get widget value
     * @param widget Widget to get value from
     * @return Widget value
     */
    static QVariant getWidgetValue(QWidget* widget);
    
    /**
     * @brief Simulate button click
     * @param button Button to click
     */
    static void clickButton(QPushButton* button);
    
    /**
     * @brief Simulate key press
     * @param widget Widget to send key to
     * @param key Key to press
     * @param modifiers Key modifiers
     */
    static void pressKey(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    
    /**
     * @brief Wait for widget to become visible
     * @param widget Widget to wait for
     * @param timeoutMs Timeout in milliseconds
     * @return true if widget became visible within timeout
     */
    static bool waitForVisible(QWidget* widget, int timeoutMs = 1000);
    
    /**
     * @brief Wait for widget to become enabled
     * @param widget Widget to wait for
     * @param timeoutMs Timeout in milliseconds
     * @return true if widget became enabled within timeout
     */
    static bool waitForEnabled(QWidget* widget, int timeoutMs = 1000);
    
    /**
     * @brief Verify widget accessibility
     * @param widget Widget to verify
     * @return true if widget is accessible
     */
    static bool verifyAccessibility(QWidget* widget);
};

#endif // TESTENHANCEDMUSICDIALOGS_H