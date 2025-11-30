#ifndef INPUTVALIDATOR_H
#define INPUTVALIDATOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

/**
 * @brief Centralized input validation and sanitization system for XFB
 * 
 * The InputValidator provides comprehensive validation and sanitization
 * for all user inputs throughout the XFB application. It includes
 * validation for file paths, SQL injection prevention, URL validation,
 * and general text sanitization.
 * 
 * @example
 * @code
 * QString safePath = InputValidator::sanitizeFilePath("/path/to/../file.mp3");
 * bool isValid = InputValidator::isValidAudioFile(safePath);
 * @endcode
 * 
 * @since XFB 2.0
 */
class InputValidator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Validation result structure
     */
    struct ValidationResult {
        bool isValid;           ///< Whether the input is valid
        QString sanitizedValue; ///< Sanitized version of the input
        QString errorMessage;   ///< Error message if validation failed
        
        ValidationResult(bool valid = false, const QString& value = QString(), const QString& error = QString())
            : isValid(valid), sanitizedValue(value), errorMessage(error) {}
    };

    /**
     * @brief File type categories for validation
     */
    enum class FileType {
        Audio,      ///< Audio files (mp3, wav, flac, etc.)
        Image,      ///< Image files (jpg, png, gif, etc.)
        Database,   ///< Database files (db, sqlite, etc.)
        Config,     ///< Configuration files (conf, ini, xml, etc.)
        Any         ///< Any file type
    };

    /**
     * @brief Text validation types
     */
    enum class TextType {
        General,        ///< General text with basic sanitization
        Filename,       ///< Filename with filesystem-safe characters
        DatabaseName,   ///< Database table/column names
        URL,           ///< URL validation
        Email,         ///< Email address validation
        Numeric,       ///< Numeric values only
        AlphaNumeric   ///< Alphanumeric characters only
    };

    explicit InputValidator(QObject* parent = nullptr);

    // File path validation and sanitization
    /**
     * @brief Validate and sanitize a file path
     * @param filePath The file path to validate
     * @param expectedType Expected file type (optional)
     * @param mustExist Whether the file must exist
     * @return ValidationResult with sanitized path and validation status
     */
    static ValidationResult validateFilePath(const QString& filePath, 
                                           FileType expectedType = FileType::Any,
                                           bool mustExist = false);

    /**
     * @brief Sanitize a file path by removing dangerous elements
     * @param filePath The file path to sanitize
     * @return Sanitized file path
     */
    static QString sanitizeFilePath(const QString& filePath);

    /**
     * @brief Check if a file path is safe (no directory traversal, etc.)
     * @param filePath The file path to check
     * @return true if the path is safe
     */
    static bool isPathSafe(const QString& filePath);

    /**
     * @brief Validate file extension against expected type
     * @param filePath The file path to check
     * @param expectedType Expected file type
     * @return true if extension matches expected type
     */
    static bool isValidFileType(const QString& filePath, FileType expectedType);

    // Text validation and sanitization
    /**
     * @brief Validate and sanitize text input
     * @param text The text to validate
     * @param type Type of text validation to apply
     * @param maxLength Maximum allowed length (0 = no limit)
     * @return ValidationResult with sanitized text and validation status
     */
    static ValidationResult validateText(const QString& text, 
                                       TextType type = TextType::General,
                                       int maxLength = 0);

    /**
     * @brief Sanitize text for safe display and storage
     * @param text The text to sanitize
     * @param type Type of sanitization to apply
     * @return Sanitized text
     */
    static QString sanitizeText(const QString& text, TextType type = TextType::General);

    /**
     * @brief Check for potential SQL injection patterns
     * @param text The text to check
     * @return true if text appears safe from SQL injection
     */
    static bool isSqlSafe(const QString& text);

    /**
     * @brief Escape text for safe SQL usage
     * @param text The text to escape
     * @return SQL-safe escaped text
     */
    static QString escapeSqlText(const QString& text);

    // Numeric validation
    /**
     * @brief Validate numeric input within range
     * @param text The text to validate as number
     * @param minValue Minimum allowed value
     * @param maxValue Maximum allowed value
     * @return ValidationResult with parsed number and validation status
     */
    static ValidationResult validateNumeric(const QString& text, 
                                          double minValue = -std::numeric_limits<double>::max(),
                                          double maxValue = std::numeric_limits<double>::max());

    /**
     * @brief Validate integer input within range
     * @param text The text to validate as integer
     * @param minValue Minimum allowed value
     * @param maxValue Maximum allowed value
     * @return ValidationResult with parsed integer and validation status
     */
    static ValidationResult validateInteger(const QString& text, 
                                          int minValue = std::numeric_limits<int>::min(),
                                          int maxValue = std::numeric_limits<int>::max());

    // URL and network validation
    /**
     * @brief Validate URL format and accessibility
     * @param url The URL to validate
     * @param allowedSchemes Allowed URL schemes (http, https, ftp, etc.)
     * @return ValidationResult with normalized URL and validation status
     */
    static ValidationResult validateUrl(const QString& url, 
                                      const QStringList& allowedSchemes = {"http", "https"});

    /**
     * @brief Validate email address format
     * @param email The email address to validate
     * @return ValidationResult with normalized email and validation status
     */
    static ValidationResult validateEmail(const QString& email);

    // Configuration validation
    /**
     * @brief Validate configuration key-value pairs
     * @param key Configuration key
     * @param value Configuration value
     * @param allowedKeys List of allowed keys (empty = allow all)
     * @return ValidationResult with sanitized key-value pair
     */
    static ValidationResult validateConfigPair(const QString& key, 
                                             const QString& value,
                                             const QStringList& allowedKeys = QStringList());

    // Batch validation
    /**
     * @brief Validate multiple file paths at once
     * @param filePaths List of file paths to validate
     * @param expectedType Expected file type for all paths
     * @param mustExist Whether files must exist
     * @return List of ValidationResults for each path
     */
    static QList<ValidationResult> validateFilePaths(const QStringList& filePaths,
                                                   FileType expectedType = FileType::Any,
                                                   bool mustExist = false);

    // Utility functions
    /**
     * @brief Get file extensions for a given file type
     * @param fileType The file type
     * @return List of valid extensions for the file type
     */
    static QStringList getValidExtensions(FileType fileType);

    /**
     * @brief Get maximum recommended length for text type
     * @param textType The text type
     * @return Recommended maximum length
     */
    static int getRecommendedMaxLength(TextType textType);

    /**
     * @brief Check if a character is safe for the given text type
     * @param character The character to check
     * @param textType The text type context
     * @return true if character is safe
     */
    static bool isCharacterSafe(QChar character, TextType textType);

    /**
     * @brief Remove or replace unsafe characters from text
     * @param text The text to clean
     * @param textType The text type context
     * @param replacement Character to replace unsafe chars with
     * @return Cleaned text
     */
    static QString removeUnsafeCharacters(const QString& text, 
                                        TextType textType,
                                        QChar replacement = '_');

private:
    // Internal validation helpers
    static bool isValidAudioExtension(const QString& extension);
    static bool isValidImageExtension(const QString& extension);
    static bool isValidDatabaseExtension(const QString& extension);
    static bool isValidConfigExtension(const QString& extension);
    
    static QString normalizeFilePath(const QString& filePath);
    static bool containsDirectoryTraversal(const QString& filePath);
    static bool containsSqlInjectionPatterns(const QString& text);
    
    // Regular expressions for validation
    static const QRegularExpression s_emailRegex;
    static const QRegularExpression s_sqlInjectionRegex;
    static const QRegularExpression s_directoryTraversalRegex;
    static const QRegularExpression s_filenameRegex;
    static const QRegularExpression s_alphaNumericRegex;
    
    // File extension lists
    static const QStringList s_audioExtensions;
    static const QStringList s_imageExtensions;
    static const QStringList s_databaseExtensions;
    static const QStringList s_configExtensions;
    
    // Text length limits
    static const int s_maxGeneralTextLength;
    static const int s_maxFilenameLength;
    static const int s_maxDatabaseNameLength;
    static const int s_maxUrlLength;
    static const int s_maxEmailLength;
};

#endif // INPUTVALIDATOR_H