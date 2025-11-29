#include "TestInputValidator.h"
#include "../../../src/services/InputValidator.h"
#include <QDir>
#include <QFile>
#include <QTextStream>

void TestInputValidator::initTestCase()
{
    // No special setup needed
}

void TestInputValidator::cleanupTestCase()
{
    // No special cleanup needed
}

void TestInputValidator::init()
{
    // Create a temporary directory for each test
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    
    createTestFiles();
}

void TestInputValidator::cleanup()
{
    // Cleanup is handled by QTemporaryDir destructor
    m_tempDir.reset();
}

void TestInputValidator::testValidateFilePathValid()
{
    auto result = InputValidator::validateFilePath(m_testAudioFile, InputValidator::FileType::Audio, true);
    QVERIFY(result.isValid);
    QVERIFY(!result.sanitizedValue.isEmpty());
    QVERIFY(result.errorMessage.isEmpty());
}

void TestInputValidator::testValidateFilePathEmpty()
{
    auto result = InputValidator::validateFilePath("", InputValidator::FileType::Any, false);
    QVERIFY(!result.isValid);
    QVERIFY(result.sanitizedValue.isEmpty());
    QVERIFY(!result.errorMessage.isEmpty());
}

void TestInputValidator::testValidateFilePathDirectoryTraversal()
{
    QString maliciousPath = "../../../etc/passwd";
    auto result = InputValidator::validateFilePath(maliciousPath, InputValidator::FileType::Any, false);
    QVERIFY(!result.isValid);
    QVERIFY(result.errorMessage.contains("unsafe"));
}

void TestInputValidator::testValidateFilePathNonExistent()
{
    QString nonExistentPath = m_tempDir->path() + "/nonexistent.mp3";
    auto result = InputValidator::validateFilePath(nonExistentPath, InputValidator::FileType::Audio, true);
    QVERIFY(!result.isValid);
    QVERIFY(result.errorMessage.contains("does not exist"));
}

void TestInputValidator::testValidateFilePathWrongType()
{
    auto result = InputValidator::validateFilePath(m_testAudioFile, InputValidator::FileType::Image, false);
    QVERIFY(!result.isValid);
    QVERIFY(result.errorMessage.contains("does not match expected type"));
}

void TestInputValidator::testSanitizeFilePath()
{
    QString dirtyPath = "  /path/to/file.mp3  ";
    QString sanitized = InputValidator::sanitizeFilePath(dirtyPath);
    QVERIFY(!sanitized.startsWith(" "));
    QVERIFY(!sanitized.endsWith(" "));
    
    // Test with null bytes
    QString pathWithNull = QString("path/to/file\0.mp3").replace('\0', QChar(0));
    QString sanitizedNull = InputValidator::sanitizeFilePath(pathWithNull);
    QVERIFY(!sanitizedNull.contains(QChar(0)));
}

void TestInputValidator::testIsPathSafe()
{
    QVERIFY(InputValidator::isPathSafe("/safe/path/file.mp3"));
    QVERIFY(!InputValidator::isPathSafe("../../../etc/passwd"));
    QVERIFY(!InputValidator::isPathSafe("path\\..\\..\\file.txt"));
    QVERIFY(!InputValidator::isPathSafe(QString("path\0file").replace('\0', QChar(0))));
}

void TestInputValidator::testIsValidFileType()
{
    QVERIFY(InputValidator::isValidFileType("file.mp3", InputValidator::FileType::Audio));
    QVERIFY(InputValidator::isValidFileType("file.jpg", InputValidator::FileType::Image));
    QVERIFY(InputValidator::isValidFileType("file.db", InputValidator::FileType::Database));
    QVERIFY(InputValidator::isValidFileType("file.conf", InputValidator::FileType::Config));
    QVERIFY(InputValidator::isValidFileType("file.xyz", InputValidator::FileType::Any));
    
    QVERIFY(!InputValidator::isValidFileType("file.mp3", InputValidator::FileType::Image));
    QVERIFY(!InputValidator::isValidFileType("file.jpg", InputValidator::FileType::Audio));
}

void TestInputValidator::testValidateTextGeneral()
{
    auto result = InputValidator::validateText("Hello World", InputValidator::TextType::General);
    QVERIFY(result.isValid);
    QCOMPARE(result.sanitizedValue, QString("Hello World"));
    
    // Test with SQL injection attempt
    auto sqlResult = InputValidator::validateText("'; DROP TABLE users; --", InputValidator::TextType::General);
    QVERIFY(!sqlResult.isValid);
    QVERIFY(sqlResult.errorMessage.contains("unsafe"));
}

void TestInputValidator::testValidateTextFilename()
{
    auto result = InputValidator::validateText("valid_filename.txt", InputValidator::TextType::Filename);
    QVERIFY(result.isValid);
    
    auto invalidResult = InputValidator::validateText("invalid/filename.txt", InputValidator::TextType::Filename);
    QVERIFY(!invalidResult.isValid);
}

void TestInputValidator::testValidateTextDatabaseName()
{
    auto result = InputValidator::validateText("valid_table_name", InputValidator::TextType::DatabaseName);
    QVERIFY(result.isValid);
    
    auto invalidResult = InputValidator::validateText("123invalid", InputValidator::TextType::DatabaseName);
    QVERIFY(!invalidResult.isValid);
    
    auto invalidResult2 = InputValidator::validateText("table-name", InputValidator::TextType::DatabaseName);
    QVERIFY(!invalidResult2.isValid);
}

void TestInputValidator::testValidateTextAlphaNumeric()
{
    auto result = InputValidator::validateText("ABC123", InputValidator::TextType::AlphaNumeric);
    QVERIFY(result.isValid);
    
    auto invalidResult = InputValidator::validateText("ABC-123", InputValidator::TextType::AlphaNumeric);
    QVERIFY(!invalidResult.isValid);
}

void TestInputValidator::testValidateTextNumeric()
{
    auto result = InputValidator::validateText("123.45", InputValidator::TextType::Numeric);
    QVERIFY(result.isValid);
    
    auto negativeResult = InputValidator::validateText("-123.45", InputValidator::TextType::Numeric);
    QVERIFY(negativeResult.isValid);
    
    auto invalidResult = InputValidator::validateText("123.45.67", InputValidator::TextType::Numeric);
    QVERIFY(!invalidResult.isValid);
}

void TestInputValidator::testValidateTextMaxLength()
{
    QString longText(1000, 'A');
    auto result = InputValidator::validateText(longText, InputValidator::TextType::General, 500);
    QVERIFY(!result.isValid);
    QCOMPARE(result.sanitizedValue.length(), 500);
}

void TestInputValidator::testSanitizeText()
{
    // Test control character removal
    QString textWithControl = QString("Hello\x01World\x7F");
    QString sanitized = InputValidator::sanitizeText(textWithControl, InputValidator::TextType::General);
    QVERIFY(!sanitized.contains(QChar(0x01)));
    QVERIFY(!sanitized.contains(QChar(0x7F)));
    
    // Test filename sanitization
    QString invalidFilename = "file<>name.txt";
    QString sanitizedFilename = InputValidator::sanitizeText(invalidFilename, InputValidator::TextType::Filename);
    QVERIFY(!sanitizedFilename.contains('<'));
    QVERIFY(!sanitizedFilename.contains('>'));
}

void TestInputValidator::testIsSqlSafe()
{
    QVERIFY(InputValidator::isSqlSafe("Safe text"));
    QVERIFY(!InputValidator::isSqlSafe("'; DROP TABLE users; --"));
    QVERIFY(!InputValidator::isSqlSafe("SELECT * FROM users"));
    QVERIFY(!InputValidator::isSqlSafe("UNION SELECT password FROM users"));
}

void TestInputValidator::testEscapeSqlText()
{
    QString textWithQuotes = "O'Reilly's book";
    QString escaped = InputValidator::escapeSqlText(textWithQuotes);
    QVERIFY(escaped.contains("''"));
    QVERIFY(!escaped.contains("'"));
}

void TestInputValidator::testSqlInjectionPatterns()
{
    QStringList maliciousInputs = {
        "'; DROP TABLE users; --",
        "1' OR '1'='1",
        "UNION SELECT * FROM passwords",
        "'; INSERT INTO users VALUES ('hacker', 'password'); --",
        "1; DELETE FROM users; --"
    };
    
    for (const QString& input : maliciousInputs) {
        QVERIFY(!InputValidator::isSqlSafe(input));
    }
}

void TestInputValidator::testValidateNumeric()
{
    auto result = InputValidator::validateNumeric("123.45");
    QVERIFY(result.isValid);
    QCOMPARE(result.sanitizedValue, QString("123.45"));
    
    auto invalidResult = InputValidator::validateNumeric("not a number");
    QVERIFY(!invalidResult.isValid);
}

void TestInputValidator::testValidateNumericRange()
{
    auto result = InputValidator::validateNumeric("50", 0, 100);
    QVERIFY(result.isValid);
    
    auto outOfRangeResult = InputValidator::validateNumeric("150", 0, 100);
    QVERIFY(!outOfRangeResult.isValid);
    QVERIFY(outOfRangeResult.errorMessage.contains("between"));
}

void TestInputValidator::testValidateInteger()
{
    auto result = InputValidator::validateInteger("42");
    QVERIFY(result.isValid);
    QCOMPARE(result.sanitizedValue, QString("42"));
    
    auto invalidResult = InputValidator::validateInteger("42.5");
    QVERIFY(!invalidResult.isValid);
}

void TestInputValidator::testValidateIntegerRange()
{
    auto result = InputValidator::validateInteger("50", 0, 100);
    QVERIFY(result.isValid);
    
    auto outOfRangeResult = InputValidator::validateInteger("150", 0, 100);
    QVERIFY(!outOfRangeResult.isValid);
}

void TestInputValidator::testValidateUrl()
{
    auto result = InputValidator::validateUrl("https://www.example.com");
    QVERIFY(result.isValid);
    
    auto result2 = InputValidator::validateUrl("http://localhost:8080/path");
    QVERIFY(result2.isValid);
}

void TestInputValidator::testValidateUrlSchemes()
{
    QStringList allowedSchemes = {"https"};
    
    auto httpsResult = InputValidator::validateUrl("https://example.com", allowedSchemes);
    QVERIFY(httpsResult.isValid);
    
    auto httpResult = InputValidator::validateUrl("http://example.com", allowedSchemes);
    QVERIFY(!httpResult.isValid);
    QVERIFY(httpResult.errorMessage.contains("not allowed"));
}

void TestInputValidator::testValidateUrlInvalid()
{
    auto result = InputValidator::validateUrl("not a url");
    QVERIFY(!result.isValid);
    
    auto result2 = InputValidator::validateUrl("http://");
    QVERIFY(!result2.isValid);
}

void TestInputValidator::testValidateEmail()
{
    auto result = InputValidator::validateEmail("user@example.com");
    QVERIFY(result.isValid);
    QCOMPARE(result.sanitizedValue, QString("user@example.com"));
    
    auto result2 = InputValidator::validateEmail("User.Name+tag@example.co.uk");
    QVERIFY(result2.isValid);
}

void TestInputValidator::testValidateEmailInvalid()
{
    QStringList invalidEmails = {
        "invalid.email",
        "@example.com",
        "user@",
        "user@.com",
        "user..name@example.com",
        "user@example",
        ""
    };
    
    for (const QString& email : invalidEmails) {
        auto result = InputValidator::validateEmail(email);
        QVERIFY2(!result.isValid, qPrintable(QString("Email '%1' should be invalid").arg(email)));
    }
}

void TestInputValidator::testValidateConfigPair()
{
    auto result = InputValidator::validateConfigPair("max_connections", "100");
    QVERIFY(result.isValid);
    QVERIFY(result.sanitizedValue.contains("max_connections=100"));
}

void TestInputValidator::testValidateConfigPairAllowedKeys()
{
    QStringList allowedKeys = {"max_connections", "timeout"};
    
    auto validResult = InputValidator::validateConfigPair("max_connections", "100", allowedKeys);
    QVERIFY(validResult.isValid);
    
    auto invalidResult = InputValidator::validateConfigPair("invalid_key", "value", allowedKeys);
    QVERIFY(!invalidResult.isValid);
    QVERIFY(invalidResult.errorMessage.contains("not allowed"));
}

void TestInputValidator::testValidateConfigPairInvalid()
{
    auto result = InputValidator::validateConfigPair("123invalid", "value");
    QVERIFY(!result.isValid);
    QVERIFY(result.errorMessage.contains("Invalid config key"));
}

void TestInputValidator::testValidateFilePaths()
{
    QStringList paths = {m_testAudioFile, m_testImageFile, "nonexistent.mp3"};
    auto results = InputValidator::validateFilePaths(paths, InputValidator::FileType::Any, false);
    
    QCOMPARE(results.size(), 3);
    QVERIFY(results[0].isValid);  // Audio file exists
    QVERIFY(results[1].isValid);  // Image file exists
    QVERIFY(results[2].isValid);  // Nonexistent file is valid when mustExist=false
}

void TestInputValidator::testGetValidExtensions()
{
    auto audioExts = InputValidator::getValidExtensions(InputValidator::FileType::Audio);
    QVERIFY(audioExts.contains("mp3"));
    QVERIFY(audioExts.contains("wav"));
    
    auto imageExts = InputValidator::getValidExtensions(InputValidator::FileType::Image);
    QVERIFY(imageExts.contains("jpg"));
    QVERIFY(imageExts.contains("png"));
}

void TestInputValidator::testGetRecommendedMaxLength()
{
    int generalLength = InputValidator::getRecommendedMaxLength(InputValidator::TextType::General);
    int filenameLength = InputValidator::getRecommendedMaxLength(InputValidator::TextType::Filename);
    
    QVERIFY(generalLength > 0);
    QVERIFY(filenameLength > 0);
    QVERIFY(generalLength > filenameLength);
}

void TestInputValidator::testIsCharacterSafe()
{
    QVERIFY(InputValidator::isCharacterSafe('A', InputValidator::TextType::AlphaNumeric));
    QVERIFY(InputValidator::isCharacterSafe('1', InputValidator::TextType::AlphaNumeric));
    QVERIFY(!InputValidator::isCharacterSafe('-', InputValidator::TextType::AlphaNumeric));
    
    QVERIFY(InputValidator::isCharacterSafe('5', InputValidator::TextType::Numeric));
    QVERIFY(InputValidator::isCharacterSafe('.', InputValidator::TextType::Numeric));
    QVERIFY(!InputValidator::isCharacterSafe('A', InputValidator::TextType::Numeric));
    
    QVERIFY(!InputValidator::isCharacterSafe(QChar(0), InputValidator::TextType::General));
    QVERIFY(!InputValidator::isCharacterSafe(QChar(0x01), InputValidator::TextType::General));
}

void TestInputValidator::testRemoveUnsafeCharacters()
{
    QString text = "ABC-123_XYZ";
    QString cleaned = InputValidator::removeUnsafeCharacters(text, InputValidator::TextType::AlphaNumeric, '_');
    
    QVERIFY(cleaned.contains('A'));
    QVERIFY(cleaned.contains('1'));
    QVERIFY(!cleaned.contains('-'));
    QVERIFY(cleaned.contains('_')); // Replacement character
}

void TestInputValidator::testNullByteHandling()
{
    QString textWithNull = QString("Hello\0World").replace('\0', QChar(0));
    auto result = InputValidator::validateText(textWithNull, InputValidator::TextType::General);
    QVERIFY(!result.sanitizedValue.contains(QChar(0)));
}

void TestInputValidator::testControlCharacterHandling()
{
    QString textWithControl = QString("Hello\x01\x02\x03World");
    auto result = InputValidator::validateText(textWithControl, InputValidator::TextType::General);
    QVERIFY(!result.sanitizedValue.contains(QChar(0x01)));
    QVERIFY(!result.sanitizedValue.contains(QChar(0x02)));
    QVERIFY(!result.sanitizedValue.contains(QChar(0x03)));
}

void TestInputValidator::testExcessiveLengthHandling()
{
    QString longText(100000, 'A');
    QString sanitized = InputValidator::sanitizeFilePath(longText);
    QVERIFY(sanitized.length() <= 4096);
}

void TestInputValidator::testUnicodeHandling()
{
    QString unicodeText = "Héllo Wörld 你好";
    auto result = InputValidator::validateText(unicodeText, InputValidator::TextType::General);
    QVERIFY(result.isValid);
    QVERIFY(result.sanitizedValue.contains("Héllo"));
}

void TestInputValidator::testPathTraversalVariations()
{
    QStringList maliciousPaths = {
        "../../../etc/passwd",
        "..\\..\\..\\windows\\system32\\config\\sam",
        "/path/to/../../../etc/passwd",
        "path/to/file/../../../etc/passwd",
        "....//....//....//etc/passwd"
    };
    
    for (const QString& path : maliciousPaths) {
        QVERIFY2(!InputValidator::isPathSafe(path), 
                qPrintable(QString("Path '%1' should be unsafe").arg(path)));
    }
}

void TestInputValidator::testSqlInjectionVariations()
{
    QStringList injectionAttempts = {
        "'; DROP TABLE users; --",
        "1' OR '1'='1' --",
        "admin'/*",
        "1; DELETE FROM users WHERE 1=1; --",
        "' UNION SELECT username, password FROM users --",
        "1' AND (SELECT COUNT(*) FROM users) > 0 --"
    };
    
    for (const QString& attempt : injectionAttempts) {
        QVERIFY2(!InputValidator::isSqlSafe(attempt),
                qPrintable(QString("SQL injection attempt '%1' should be detected").arg(attempt)));
    }
}

void TestInputValidator::createTestFiles()
{
    m_testAudioFile = createTestFile("mp3", "fake audio content");
    m_testImageFile = createTestFile("jpg", "fake image content");
    m_testDatabaseFile = createTestFile("db", "fake database content");
    m_testConfigFile = createTestFile("conf", "key=value\nother_key=other_value");
}

QString TestInputValidator::createTestFile(const QString& extension, const QString& content)
{
    QString fileName = QString("test.%1").arg(extension);
    QString filePath = m_tempDir->path() + "/" + fileName;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << content;
        file.close();
    }
    
    return filePath;
}

QTEST_MAIN(TestInputValidator)