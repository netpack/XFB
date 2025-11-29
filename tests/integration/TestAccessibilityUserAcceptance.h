#ifndef TESTACCESSIBILITYUSERACCEPTANCE_H
#define TESTACCESSIBILITYUSERACCEPTANCE_H

#include <QObject>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QStringList>
#include <QHash>

class QWidget;
class QMainWindow;
class AccessibilityManager;

/**
 * @brief User Acceptance Testing framework for accessibility features
 * 
 * Provides comprehensive testing scenarios that simulate real-world
 * usage by visually impaired broadcasters using ORCA screen reader.
 */
class TestAccessibilityUserAcceptance : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Test scenario types
     */
    enum TestScenario {
        RadioShowSetup,         ///< Setting up for a radio show
        MusicLibraryManagement, ///< Managing music library
        LiveBroadcasting,       ///< Live broadcasting workflow
        PlaylistCreation,       ///< Creating and managing playlists
        EmergencyProcedures,    ///< Emergency stop and recovery
        SystemConfiguration,    ///< Configuring accessibility settings
        HelpAndDocumentation    ///< Using help system
    };

    /**
     * @brief Test result structure
     */
    struct TestResult {
        TestScenario scenario;
        QString scenarioName;
        bool passed;
        double completionTime;
        int keystrokes;
        int announcements;
        QStringList issues;
        QStringList positiveNotes;
        QString userFeedback;
    };

    explicit TestAccessibilityUserAcceptance(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Complete workflow tests
    void testRadioShowSetupWorkflow();
    void testMusicLibraryManagementWorkflow();
    void testLiveBroadcastingWorkflow();
    void testPlaylistCreationWorkflow();
    void testEmergencyProceduresWorkflow();
    void testSystemConfigurationWorkflow();
    void testHelpAndDocumentationWorkflow();

    // Real-world scenario tests
    void testCompleteRadioShowFromSetupToBroadcast();
    void testAccessibilityUnderStress();
    void testKeyboardOnlyOperation();
    void testScreenReaderCompatibility();
    void testBrailleDisplaySupport();

    // Performance and usability tests
    void testAnnouncementClarity();
    void testNavigationEfficiency();
    void testErrorRecovery();
    void testUserCustomization();

private:
    // Test scenario implementations
    TestResult executeRadioShowSetup();
    TestResult executeMusicLibraryManagement();
    TestResult executeLiveBroadcasting();
    TestResult executePlaylistCreation();
    TestResult executeEmergencyProcedures();
    TestResult executeSystemConfiguration();
    TestResult executeHelpAndDocumentation();

    // Helper methods
    void setupTestEnvironment();
    void createTestBroadcastingInterface();
    void simulateUserInteraction(const QStringList& actions);
    void measureUserExperience(TestScenario scenario);
    void collectUserFeedback(TestScenario scenario);
    void validateAccessibilityCompliance();
    void generateTestReport();

    // Measurement and validation
    void startScenarioMeasurement(TestScenario scenario);
    void endScenarioMeasurement(TestScenario scenario);
    bool validateScenarioCompletion(TestScenario scenario);
    void recordUserAction(const QString& action);
    void recordAnnouncement(const QString& announcement);

    // Test data and fixtures
    QMainWindow* m_testMainWindow;
    AccessibilityManager* m_accessibilityManager;
    
    QHash<TestScenario, TestResult> m_testResults;
    QElapsedTimer m_scenarioTimer;
    TestScenario m_currentScenario;
    int m_keystrokeCount;
    int m_announcementCount;
    QStringList m_userActions;
    QStringList m_announcements;
    
    // Test configuration
    bool m_orcaAvailable;
    bool m_brailleAvailable;
    bool m_interactiveMode;
};

Q_DECLARE_METATYPE(TestAccessibilityUserAcceptance::TestScenario)
Q_DECLARE_METATYPE(TestAccessibilityUserAcceptance::TestResult)

#endif // TESTACCESSIBILITYUSERACCEPTANCE_H