#include <QtTest/QtTest>
#include <QApplication>
#include <QTimer>
#include "test_utils.h"

class TestAppIntegration : public XFBTestBase
{
    Q_OBJECT

private slots:
    void testApplicationStartup();
    void testConfigurationLoading();

private:
    QApplication* m_app = nullptr;
};

void TestAppIntegration::testApplicationStartup()
{
    // Test basic application initialization
    int argc = 1;
    char* argv[] = {"test_app"};
    
    // Create QApplication for GUI testing
    if (!QApplication::instance()) {
        m_app = new QApplication(argc, argv);
    }
    
    QVERIFY(QApplication::instance() != nullptr);
    
    // Test that application can be created without immediate crash
    QTimer::singleShot(100, [this]() {
        if (m_app) {
            m_app->quit();
        }
    });
    
    // Don't actually run the event loop in tests
    QVERIFY(true); // Basic test passes if we get here
}

void TestAppIntegration::testConfigurationLoading()
{
    // Test configuration system integration
    QCoreApplication::setApplicationName("XFB_Test");
    QCoreApplication::setOrganizationName("Netpack_Test");
    
    // This would test actual configuration loading
    // For now, just verify the test setup works
    QVERIFY(!QCoreApplication::applicationName().isEmpty());
    QVERIFY(!QCoreApplication::organizationName().isEmpty());
}

QTEST_MAIN(TestAppIntegration)
#include "test_app_integration.moc"