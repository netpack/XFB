#ifndef TESTORCACOMPATIBILITY_H
#define TESTORCACOMPATIBILITY_H

#include <QObject>
#include <QTest>
#include <QProcess>
#include <QTimer>
#include <QAccessible>
#include <QAccessibleInterface>

/**
 * @brief ORCA screen reader compatibility tests
 * 
 * Tests specific ORCA integration scenarios and compatibility
 * with AT-SPI interface requirements.
 */
class TestORCACompatibility : public QObject
{
    Q_OBJECT

public:
    TestORCACompatibility();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // ORCA-specific tests
    void testORCAScreenReaderDetection();
    void testATSPIInterfaceCompliance();
    void testORCAAnnouncementFormats();
    void testORCANavigationPatterns();
    void testORCABrailleOutput();

    // WCAG 2.1 AA compliance tests
    void testWCAGKeyboardAccessibility();
    void testWCAGFocusManagement();
    void testWCAGColorContrast();
    void testWCAGTextAlternatives();
    void testWCAGTimingAdjustable();

    // Keyboard-only operation tests
    void testCompleteKeyboardOnlyWorkflow();
    void testKeyboardShortcutCoverage();
    void testKeyboardTrapAvoidance();
    void testFocusIndicatorVisibility();

private:
    // Helper methods
    bool isORCARunning();
    void startORCAIfAvailable();
    void stopORCA();
    void verifyATSPIConnection();
    void testAccessibleInterface(QWidget* widget);
    void verifyWCAGCompliance(QWidget* widget);
    
    QProcess* m_orcaProcess;
    bool m_orcaAvailable;
    bool m_atSpiConnected;
};

#endif // TESTORCACOMPATIBILITY_H