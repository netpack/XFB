#ifndef TESTINPUTVALIDATOR_H
#define TESTINPUTVALIDATOR_H

#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <memory>

class InputValidator;

/**
 * @brief Unit tests for InputValidator class
 * 
 * Tests the comprehensive input validation and sanitization system including:
 * - File path validation and sanitization
 * - Text validation for different types
 * - SQL injection prevention
 * - URL and email validation
 * - Numeric validation
 * - Configuration validation
 */
class TestInputValidator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // File path validation tests
    void testValidateFilePathValid();
    void testValidateFilePathEmpty();
    void testValidateFilePathDirectoryTraversal();
    void testValidateFilePathNonExistent();
    void testValidateFilePathWrongType();
    void testSanitizeFilePath();
    void testIsPathSafe();
    void testIsValidFileType();

    // Text validation tests
    void testValidateTextGeneral();
    void testValidateTextFilename();
    void testValidateTextDatabaseName();
    void testValidateTextAlphaNumeric();
    void testValidateTextNumeric();
    void testValidateTextMaxLength();
    void testSanitizeText();

    // SQL injection prevention tests
    void testIsSqlSafe();
    void testEscapeSqlText();
    void testSqlInjectionPatterns();

    // Numeric validation tests
    void testValidateNumeric();
    void testValidateNumericRange();
    void testValidateInteger();
    void testValidateIntegerRange();

    // URL validation tests
    void testValidateUrl();
    void testValidateUrlSchemes();
    void testValidateUrlInvalid();

    // Email validation tests
    void testValidateEmail();
    void testValidateEmailInvalid();

    // Configuration validation tests
    void testValidateConfigPair();
    void testValidateConfigPairAllowedKeys();
    void testValidateConfigPairInvalid();

    // Batch validation tests
    void testValidateFilePaths();

    // Utility function tests
    void testGetValidExtensions();
    void testGetRecommendedMaxLength();
    void testIsCharacterSafe();
    void testRemoveUnsafeCharacters();

    // Edge cases and security tests
    void testNullByteHandling();
    void testControlCharacterHandling();
    void testExcessiveLengthHandling();
    void testUnicodeHandling();
    void testPathTraversalVariations();
    void testSqlInjectionVariations();

private:
    void createTestFiles();
    QString createTestFile(const QString& extension, const QString& content = "test");
    
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_testAudioFile;
    QString m_testImageFile;
    QString m_testDatabaseFile;
    QString m_testConfigFile;
};

#endif // TESTINPUTVALIDATOR_H