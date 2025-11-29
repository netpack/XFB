#include <QtTest/QtTest>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include "../../../src/services/AccessibilityManager.h"
#include "../../../src/services/ServiceContainer.h"

/**
 * @brief Unit tests for AccessibilityManager
 */
class TestAccessibilityManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testInitialization();
    void testEnableDisableAccessibility();
    void testVerbosityLevels();
    void testWidgetRegistration();
    void testAnnouncements();
    void testSettingsPersistence();

private:
    AccessibilityManager* m_accessibilityManager;
    ServiceContainer* m_serviceContainer;
};

void TestAccessibilityManager::initTestCase()
{
    // Initialize service container
    m_serviceContainer = ServiceContainer::instance();
    
    // Register AccessibilityManager
    m_serviceContainer->registerSingleton<AccessibilityManager>();
    
    // Initialize services
    QVERIFY(m_serviceContainer->initializeServices());
}

void TestAccessibilityManager::cleanupTestCase()
{
    if (m_serviceContainer) {
        m_serviceContainer->shutdownServices();
        ServiceContainer::destroyInstance();
    }
}

void TestAccessibilityManager::init()
{
    m_accessibilityManager = m_serviceContainer->resolve<AccessibilityManager>();
    QVERIFY(m_accessibilityManager != nullptr);
}

void TestAccessibilityManager::cleanup()
{
    // Reset to default state
    if (m_accessibilityManager) {
        m_accessibilityManager->enableAccessibility(false);
        m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Normal);
    }
}

void TestAccessibilityManager::testInitialization()
{
    QVERIFY(m_accessibilityManager->isRunning());
    QCOMPARE(m_accessibilityManager->serviceName(), QString("AccessibilityManager"));
    
    // Test initial state
    QVERIFY(!m_accessibilityManager->isAccessibilityEnabled()); // Should be disabled by default
    QCOMPARE(m_accessibilityManager->verbosityLevel(), AccessibilityManager::VerbosityLevel::Normal);
}

void TestAccessibilityManager::testEnableDisableAccessibility()
{
    QSignalSpy spy(m_accessibilityManager, &AccessibilityManager::accessibilityStateChanged);
    
    // Test enabling accessibility
    m_accessibilityManager->enableAccessibility(true);
    QVERIFY(m_accessibilityManager->isAccessibilityEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    
    // Test disabling accessibility
    m_accessibilityManager->enableAccessibility(false);
    QVERIFY(!m_accessibilityManager->isAccessibilityEnabled());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), false);
    
    // Test no change when setting same state
    spy.clear();
    m_accessibilityManager->enableAccessibility(false);
    QCOMPARE(spy.count(), 0); // No signal should be emitted
}

void TestAccessibilityManager::testVerbosityLevels()
{
    QSignalSpy spy(m_accessibilityManager, &AccessibilityManager::verbosityLevelChanged);
    
    // Test setting different verbosity levels
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Terse);
    QCOMPARE(m_accessibilityManager->verbosityLevel(), AccessibilityManager::VerbosityLevel::Terse);
    QCOMPARE(spy.count(), 1);
    
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Verbose);
    QCOMPARE(m_accessibilityManager->verbosityLevel(), AccessibilityManager::VerbosityLevel::Verbose);
    QCOMPARE(spy.count(), 2);
    
    // Test no change when setting same level
    spy.clear();
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Verbose);
    QCOMPARE(spy.count(), 0); // No signal should be emitted
}

void TestAccessibilityManager::testWidgetRegistration()
{
    // Create a test widget
    QPushButton* testButton = new QPushButton("Test Button");
    
    // Create metadata
    AccessibilityManager::AccessibilityMetadata metadata;
    metadata.name = "Test Button";
    metadata.description = "A test button for accessibility";
    metadata.role = QAccessible::Button;
    metadata.helpText = "Press to test";
    
    // Register widget
    m_accessibilityManager->registerAccessibleWidget(testButton, metadata);
    
    // Verify metadata can be retrieved
    auto retrievedMetadata = m_accessibilityManager->getAccessibilityMetadata(testButton);
    QCOMPARE(retrievedMetadata.name, metadata.name);
    QCOMPARE(retrievedMetadata.description, metadata.description);
    QCOMPARE(retrievedMetadata.role, metadata.role);
    QCOMPARE(retrievedMetadata.helpText, metadata.helpText);
    
    // Test unregistration
    m_accessibilityManager->unregisterAccessibleWidget(testButton);
    auto emptyMetadata = m_accessibilityManager->getAccessibilityMetadata(testButton);
    QVERIFY(emptyMetadata.name.isEmpty());
    
    // Cleanup
    delete testButton;
}

void TestAccessibilityManager::testAnnouncements()
{
    QSignalSpy spy(m_accessibilityManager, &AccessibilityManager::announcementRequested);
    
    // Enable accessibility first
    m_accessibilityManager->enableAccessibility(true);
    spy.clear(); // Clear the signal from enabling accessibility
    
    // Test announcement
    QString testMessage = "Test announcement";
    m_accessibilityManager->announceMessage(testMessage, AccessibilityManager::Priority::Normal);
    
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), testMessage);
    QCOMPARE(spy.at(0).at(1).value<AccessibilityManager::Priority>(), AccessibilityManager::Priority::Normal);
    
    // Test that announcements are ignored when accessibility is disabled
    spy.clear();
    m_accessibilityManager->enableAccessibility(false);
    spy.clear(); // Clear the signal from disabling accessibility
    m_accessibilityManager->announceMessage("Should be ignored", AccessibilityManager::Priority::High);
    QCOMPARE(spy.count(), 0);
}

void TestAccessibilityManager::testSettingsPersistence()
{
    // Set some settings
    m_accessibilityManager->enableAccessibility(true);
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Verbose);
    
    // Save settings
    m_accessibilityManager->saveSettings();
    
    // Change settings
    m_accessibilityManager->enableAccessibility(false);
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::VerbosityLevel::Terse);
    
    // Load settings - should restore previous values
    m_accessibilityManager->loadSettings();
    
    QVERIFY(m_accessibilityManager->isAccessibilityEnabled());
    QCOMPARE(m_accessibilityManager->verbosityLevel(), AccessibilityManager::VerbosityLevel::Verbose);
}

QTEST_MAIN(TestAccessibilityManager)
#include "TestAccessibilityManager.moc"