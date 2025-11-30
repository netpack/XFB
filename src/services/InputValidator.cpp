#include "InputValidator.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QRegularExpression>
#include <QStandardPaths>
#include <limits>

// Static member definitions
const QRegularExpression InputValidator::s_emailRegex(
    R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
);

const QRegularExpression InputValidator::s_sqlInjectionRegex(
    R"((\b(SELECT|INSERT|UPDATE|DELETE|DROP|CREATE|ALTER|EXEC|UNION|SCRIPT)\b)|(')|(--)|(;)|(\/\*))",
    QRegularExpression::CaseInsensitiveOption
);

const QRegularExpression InputValidator::s_directoryTraversalRegex(
    R"((\.\.[\\/])|(\.\.[\\\/])|([\\\/]\.\.)|(^\.\.)|(\.\.$))"
);

const QRegularExpression InputValidator::s_filenameRegex(
    R"(^[a-zA-Z0-9._\-\s()]+$)"
);

const QRegularExpression InputValidator::s_alphaNumericRegex(
    R"(^[a-zA-Z0-9]+$)"
);

const QStringList InputValidator::s_audioExtensions = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "aiff", "au", "ra"
};

const QStringList InputValidator::s_imageExtensions = {
    "jpg", "jpeg", "png", "gif", "bmp", "tiff", "tga", "webp", "svg", "ico"
};

const QStringList InputValidator::s_databaseExtensions = {
    "db", "sqlite", "sqlite3", "mdb", "accdb"
};

const QStringList InputValidator::s_configExtensions = {
    "conf", "config", "ini", "cfg", "xml", "json", "yaml", "yml", "properties"
};

const int InputValidator::s_maxGeneralTextLength = 10000;
const int InputValidator::s_maxFilenameLength = 255;
const int InputValidator::s_maxDatabaseNameLength = 64;
const int InputValidator::s_maxUrlLength = 2048;
const int InputValidator::s_maxEmailLength = 254;

InputValidator::InputValidator(QObject* parent)
    : QObject(parent)
{
}

InputValidator::ValidationResult InputValidator::validateFilePath(const QString& filePath, 
                                                                FileType expectedType,
                                                                bool mustExist)
{
    if (filePath.isEmpty()) {
        return ValidationResult(false, QString(), "File path cannot be empty");
    }

    // Check for directory traversal attacks
    if (!isPathSafe(filePath)) {
        return ValidationResult(false, QString(), "File path contains unsafe directory traversal patterns");
    }

    // Sanitize the path
    QString sanitizedPath = sanitizeFilePath(filePath);
    if (sanitizedPath.isEmpty()) {
        return ValidationResult(false, QString(), "File path could not be sanitized");
    }

    // Validate file type if specified
    if (expectedType != FileType::Any && !isValidFileType(sanitizedPath, expectedType)) {
        return ValidationResult(false, sanitizedPath, 
                              QString("File type does not match expected type"));
    }

    // Check if file exists if required
    if (mustExist) {
        QFileInfo fileInfo(sanitizedPath);
        if (!fileInfo.exists()) {
            return ValidationResult(false, sanitizedPath, "File does not exist");
        }
        if (!fileInfo.isReadable()) {
            return ValidationResult(false, sanitizedPath, "File is not readable");
        }
    }

    return ValidationResult(true, sanitizedPath, QString());
}

QString InputValidator::sanitizeFilePath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QString();
    }

    QString sanitized = filePath.trimmed();
    
    // Normalize path separators
    sanitized = QDir::cleanPath(sanitized);
    
    // Remove null bytes and other control characters
    sanitized.remove(QChar('\0'));
    sanitized.remove(QRegularExpression(R"([\x00-\x1F\x7F])"));
    
    // Limit path length
    if (sanitized.length() > 4096) { // Reasonable path length limit
        sanitized = sanitized.left(4096);
    }
    
    return sanitized;
}

bool InputValidator::isPathSafe(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    // Check for directory traversal patterns
    if (containsDirectoryTraversal(filePath)) {
        return false;
    }

    // Check for null bytes
    if (filePath.contains(QChar('\0'))) {
        return false;
    }

    // Check for excessive length
    if (filePath.length() > 4096) {
        return false;
    }

    return true;
}

bool InputValidator::isValidFileType(const QString& filePath, FileType expectedType)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    switch (expectedType) {
    case FileType::Audio:
        return isValidAudioExtension(extension);
    case FileType::Image:
        return isValidImageExtension(extension);
    case FileType::Database:
        return isValidDatabaseExtension(extension);
    case FileType::Config:
        return isValidConfigExtension(extension);
    case FileType::Any:
        return true;
    }
    
    return false;
}

InputValidator::ValidationResult InputValidator::validateText(const QString& text, 
                                                            TextType type,
                                                            int maxLength)
{
    if (text.isNull()) {
        return ValidationResult(false, QString(), "Text cannot be null");
    }

    // Apply default max length if not specified
    if (maxLength <= 0) {
        maxLength = getRecommendedMaxLength(type);
    }

    // Check length
    if (text.length() > maxLength) {
        return ValidationResult(false, text.left(maxLength), 
                              QString("Text exceeds maximum length of %1 characters").arg(maxLength));
    }

    // Sanitize based on type
    QString sanitized = sanitizeText(text, type);
    
    // Validate based on type
    bool isValid = true;
    QString errorMessage;

    switch (type) {
    case TextType::Email:
        isValid = s_emailRegex.match(sanitized).hasMatch();
        if (!isValid) errorMessage = "Invalid email format";
        break;
    case TextType::URL:
        {
            ValidationResult urlResult = validateUrl(sanitized);
            isValid = urlResult.isValid;
            errorMessage = urlResult.errorMessage;
            sanitized = urlResult.sanitizedValue;
        }
        break;
    case TextType::Numeric:
        {
            ValidationResult numResult = validateNumeric(sanitized);
            isValid = numResult.isValid;
            errorMessage = numResult.errorMessage;
        }
        break;
    case TextType::AlphaNumeric:
        isValid = s_alphaNumericRegex.match(sanitized).hasMatch();
        if (!isValid) errorMessage = "Text must contain only alphanumeric characters";
        break;
    case TextType::Filename:
        isValid = s_filenameRegex.match(sanitized).hasMatch();
        if (!isValid) errorMessage = "Invalid filename characters";
        break;
    case TextType::DatabaseName:
        // Database names should be alphanumeric with underscores
        isValid = QRegularExpression(R"(^[a-zA-Z][a-zA-Z0-9_]*$)").match(sanitized).hasMatch();
        if (!isValid) errorMessage = "Database name must start with letter and contain only letters, numbers, and underscores";
        break;
    case TextType::General:
    default:
        // General text is valid if it doesn't contain SQL injection patterns
        isValid = isSqlSafe(sanitized);
        if (!isValid) errorMessage = "Text contains potentially unsafe patterns";
        break;
    }

    return ValidationResult(isValid, sanitized, errorMessage);
}

QString InputValidator::sanitizeText(const QString& text, TextType type)
{
    if (text.isEmpty()) {
        return text;
    }

    QString sanitized = text;

    // Remove null bytes and control characters
    sanitized.remove(QChar('\0'));
    sanitized.remove(QRegularExpression(R"([\x00-\x08\x0B\x0C\x0E-\x1F\x7F])"));

    switch (type) {
    case TextType::Filename:
        // Keep only safe filename characters
        sanitized = removeUnsafeCharacters(sanitized, type, '_');
        break;
    case TextType::DatabaseName:
        // Keep only alphanumeric and underscore
        sanitized.remove(QRegularExpression(R"([^a-zA-Z0-9_])"));
        break;
    case TextType::AlphaNumeric:
        // Keep only alphanumeric
        sanitized.remove(QRegularExpression(R"([^a-zA-Z0-9])"));
        break;
    case TextType::Numeric:
        // Keep only numeric characters, decimal point, and minus sign
        sanitized.remove(QRegularExpression(R"([^0-9.\-])"));
        break;
    case TextType::Email:
    case TextType::URL:
        // Trim whitespace for these types
        sanitized = sanitized.trimmed();
        break;
    case TextType::General:
    default:
        // For general text, just trim and normalize whitespace
        sanitized = sanitized.trimmed();
        sanitized.replace(QRegularExpression(R"(\s+)"), " ");
        break;
    }

    return sanitized;
}

bool InputValidator::isSqlSafe(const QString& text)
{
    return !containsSqlInjectionPatterns(text);
}

QString InputValidator::escapeSqlText(const QString& text)
{
    QString escaped = text;
    
    // Escape single quotes by doubling them
    escaped.replace("'", "''");
    
    // Remove or escape other potentially dangerous characters
    escaped.remove(QChar('\0'));
    escaped.replace("\\", "\\\\");
    
    return escaped;
}

InputValidator::ValidationResult InputValidator::validateNumeric(const QString& text, 
                                                               double minValue,
                                                               double maxValue)
{
    if (text.isEmpty()) {
        return ValidationResult(false, QString(), "Numeric value cannot be empty");
    }

    bool ok;
    double value = text.toDouble(&ok);
    
    if (!ok) {
        return ValidationResult(false, text, "Invalid numeric format");
    }

    if (value < minValue || value > maxValue) {
        return ValidationResult(false, QString::number(value), 
                              QString("Value must be between %1 and %2").arg(minValue).arg(maxValue));
    }

    return ValidationResult(true, QString::number(value), QString());
}

InputValidator::ValidationResult InputValidator::validateInteger(const QString& text, 
                                                               int minValue,
                                                               int maxValue)
{
    if (text.isEmpty()) {
        return ValidationResult(false, QString(), "Integer value cannot be empty");
    }

    bool ok;
    int value = text.toInt(&ok);
    
    if (!ok) {
        return ValidationResult(false, text, "Invalid integer format");
    }

    if (value < minValue || value > maxValue) {
        return ValidationResult(false, QString::number(value), 
                              QString("Value must be between %1 and %2").arg(minValue).arg(maxValue));
    }

    return ValidationResult(true, QString::number(value), QString());
}

InputValidator::ValidationResult InputValidator::validateUrl(const QString& url, 
                                                           const QStringList& allowedSchemes)
{
    if (url.isEmpty()) {
        return ValidationResult(false, QString(), "URL cannot be empty");
    }

    QUrl qurl(url);
    if (!qurl.isValid()) {
        return ValidationResult(false, url, "Invalid URL format");
    }

    QString scheme = qurl.scheme().toLower();
    if (!allowedSchemes.isEmpty() && !allowedSchemes.contains(scheme)) {
        return ValidationResult(false, url, 
                              QString("URL scheme '%1' is not allowed").arg(scheme));
    }

    if (qurl.host().isEmpty() && (scheme == "http" || scheme == "https")) {
        return ValidationResult(false, url, "URL must have a valid host");
    }

    return ValidationResult(true, qurl.toString(), QString());
}

InputValidator::ValidationResult InputValidator::validateEmail(const QString& email)
{
    if (email.isEmpty()) {
        return ValidationResult(false, QString(), "Email cannot be empty");
    }

    QString trimmedEmail = email.trimmed().toLower();
    
    if (trimmedEmail.length() > s_maxEmailLength) {
        return ValidationResult(false, trimmedEmail, "Email address is too long");
    }

    if (!s_emailRegex.match(trimmedEmail).hasMatch()) {
        return ValidationResult(false, trimmedEmail, "Invalid email format");
    }

    return ValidationResult(true, trimmedEmail, QString());
}

InputValidator::ValidationResult InputValidator::validateConfigPair(const QString& key, 
                                                                  const QString& value,
                                                                  const QStringList& allowedKeys)
{
    // Validate key
    ValidationResult keyResult = validateText(key, TextType::DatabaseName, 64);
    if (!keyResult.isValid) {
        return ValidationResult(false, QString(), 
                              QString("Invalid config key: %1").arg(keyResult.errorMessage));
    }

    // Check if key is in allowed list
    if (!allowedKeys.isEmpty() && !allowedKeys.contains(keyResult.sanitizedValue)) {
        return ValidationResult(false, QString(), 
                              QString("Config key '%1' is not allowed").arg(keyResult.sanitizedValue));
    }

    // Validate value
    ValidationResult valueResult = validateText(value, TextType::General, 1024);
    if (!valueResult.isValid) {
        return ValidationResult(false, QString(), 
                              QString("Invalid config value: %1").arg(valueResult.errorMessage));
    }

    QString result = QString("%1=%2").arg(keyResult.sanitizedValue, valueResult.sanitizedValue);
    return ValidationResult(true, result, QString());
}

QList<InputValidator::ValidationResult> InputValidator::validateFilePaths(const QStringList& filePaths,
                                                                        FileType expectedType,
                                                                        bool mustExist)
{
    QList<ValidationResult> results;
    
    for (const QString& filePath : filePaths) {
        results.append(validateFilePath(filePath, expectedType, mustExist));
    }
    
    return results;
}

QStringList InputValidator::getValidExtensions(FileType fileType)
{
    switch (fileType) {
    case FileType::Audio:
        return s_audioExtensions;
    case FileType::Image:
        return s_imageExtensions;
    case FileType::Database:
        return s_databaseExtensions;
    case FileType::Config:
        return s_configExtensions;
    case FileType::Any:
        return QStringList();
    }
    
    return QStringList();
}

int InputValidator::getRecommendedMaxLength(TextType textType)
{
    switch (textType) {
    case TextType::General:
        return s_maxGeneralTextLength;
    case TextType::Filename:
        return s_maxFilenameLength;
    case TextType::DatabaseName:
        return s_maxDatabaseNameLength;
    case TextType::URL:
        return s_maxUrlLength;
    case TextType::Email:
        return s_maxEmailLength;
    case TextType::Numeric:
    case TextType::AlphaNumeric:
        return 50; // Reasonable default for these types
    }
    
    return s_maxGeneralTextLength;
}

bool InputValidator::isCharacterSafe(QChar character, TextType textType)
{
    // Check for control characters and null bytes
    if (character.isNull() || character.category() == QChar::Other_Control) {
        return false;
    }

    switch (textType) {
    case TextType::Filename:
        return s_filenameRegex.match(QString(character)).hasMatch();
    case TextType::AlphaNumeric:
        return character.isLetterOrNumber();
    case TextType::Numeric:
        return character.isDigit() || character == '.' || character == '-';
    case TextType::DatabaseName:
        return character.isLetterOrNumber() || character == '_';
    case TextType::General:
    case TextType::Email:
    case TextType::URL:
    default:
        return character.isPrint();
    }
}

QString InputValidator::removeUnsafeCharacters(const QString& text, 
                                             TextType textType,
                                             QChar replacement)
{
    QString result;
    result.reserve(text.length());
    
    for (const QChar& ch : text) {
        if (isCharacterSafe(ch, textType)) {
            result.append(ch);
        } else if (replacement != QChar::Null) {
            result.append(replacement);
        }
    }
    
    return result;
}

// Private helper methods
bool InputValidator::isValidAudioExtension(const QString& extension)
{
    return s_audioExtensions.contains(extension, Qt::CaseInsensitive);
}

bool InputValidator::isValidImageExtension(const QString& extension)
{
    return s_imageExtensions.contains(extension, Qt::CaseInsensitive);
}

bool InputValidator::isValidDatabaseExtension(const QString& extension)
{
    return s_databaseExtensions.contains(extension, Qt::CaseInsensitive);
}

bool InputValidator::isValidConfigExtension(const QString& extension)
{
    return s_configExtensions.contains(extension, Qt::CaseInsensitive);
}

bool InputValidator::containsDirectoryTraversal(const QString& filePath)
{
    return s_directoryTraversalRegex.match(filePath).hasMatch();
}

bool InputValidator::containsSqlInjectionPatterns(const QString& text)
{
    return s_sqlInjectionRegex.match(text).hasMatch();
}