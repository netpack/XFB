#ifndef ENHANCEDADDMUSICSINGLEDDIALOG_H
#define ENHANCEDADDMUSICSINGLEDDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QFutureWatcher>
#include <memory>

// Forward declarations
class MusicRepository;
class InputValidator;
struct MusicItem;
class QLineEdit;
class QComboBox;
class QPushButton;
class QDateEdit;
class QTextEdit;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;

/**
 * @brief Enhanced dialog for adding single music files with validation
 * 
 * This dialog provides a modern, user-friendly interface for adding
 * individual music files to the library with comprehensive validation,
 * progress indication, and error handling.
 * 
 * Features:
 * - Real-time input validation
 * - Automatic metadata extraction
 * - Progress indication for long operations
 * - Comprehensive error handling
 * - Modern UI design
 * - Accessibility support
 * 
 * @since XFB 2.0
 */
class EnhancedAddMusicSingleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EnhancedAddMusicSingleDialog(MusicRepository* repository, QWidget* parent = nullptr);
    ~EnhancedAddMusicSingleDialog();

    /**
     * @brief Set the initial file path
     * @param filePath Path to the music file
     */
    void setFilePath(const QString& filePath);

    /**
     * @brief Get the ID of the added music item
     * @return Music ID (0 if not added)
     */
    int addedMusicId() const;

public slots:
    /**
     * @brief Accept the dialog and add the music
     */
    void accept() override;

    /**
     * @brief Reject the dialog
     */
    void reject() override;

signals:
    /**
     * @brief Emitted when music is successfully added
     * @param musicId ID of the added music
     */
    void musicAdded(int musicId);

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief Handle file browse button click
     */
    void onBrowseFileClicked();

    /**
     * @brief Handle file path change
     */
    void onFilePathChanged();

    /**
     * @brief Handle input field changes for validation
     */
    void onInputChanged();

    /**
     * @brief Handle genre management button click
     */
    void onManageGenresClicked();

    /**
     * @brief Handle metadata extraction completion
     */
    void onMetadataExtractionCompleted();

    /**
     * @brief Handle validation timer timeout
     */
    void onValidationTimer();

    /**
     * @brief Handle add button click
     */
    void onAddButtonClicked();

    /**
     * @brief Handle cancel button click
     */
    void onCancelButtonClicked();

    /**
     * @brief Handle clear form button click
     */
    void onClearFormClicked();

    /**
     * @brief Handle auto-fill from metadata button click
     */
    void onAutoFillClicked();

private:
    /**
     * @brief Setup the UI layout and components
     */
    void setupUI();

    /**
     * @brief Setup validation for input fields
     */
    void setupValidation();

    /**
     * @brief Setup signal connections
     */
    void setupConnections();

    /**
     * @brief Load genre data into combo boxes
     */
    void loadGenres();

    /**
     * @brief Validate all input fields
     * @return true if all inputs are valid
     */
    bool validateInputs();

    /**
     * @brief Validate file path
     * @return true if file path is valid
     */
    bool validateFilePath();

    /**
     * @brief Validate required text fields
     * @return true if all required fields are filled
     */
    bool validateRequiredFields();

    /**
     * @brief Update validation status display
     */
    void updateValidationStatus();

    /**
     * @brief Show validation error for a specific field
     * @param field Field widget
     * @param error Error message
     */
    void showFieldError(QWidget* field, const QString& error);

    /**
     * @brief Clear validation error for a specific field
     * @param field Field widget
     */
    void clearFieldError(QWidget* field);

    /**
     * @brief Extract metadata from the selected file
     */
    void extractMetadata();

    /**
     * @brief Extract metadata asynchronously
     * @param filePath Path to the music file
     */
    void extractMetadataAsync(const QString& filePath);

    /**
     * @brief Apply extracted metadata to form fields
     * @param metadata Extracted metadata
     */
    void applyMetadata(const QVariantMap& metadata);

    /**
     * @brief Show progress indication
     * @param message Progress message
     */
    void showProgress(const QString& message);

    /**
     * @brief Hide progress indication
     */
    void hideProgress();

    /**
     * @brief Update progress
     * @param value Progress value (0-100)
     * @param message Optional progress message
     */
    void updateProgress(int value, const QString& message = QString());

    /**
     * @brief Add the music to the repository
     * @return true if music was added successfully
     */
    bool addMusic();

    /**
     * @brief Create music item from form data
     * @return Music item with form data
     */
    MusicItem createMusicItemFromForm() const;

    /**
     * @brief Reset form to default state
     */
    void resetForm();

    /**
     * @brief Enable or disable form controls
     * @param enabled true to enable controls
     */
    void setFormEnabled(bool enabled);
    
    /**
     * @brief Load countries into the country combo box
     */
    void loadCountries();

    /**
     * @brief Get supported audio file extensions
     * @return List of supported extensions
     */
    QStringList getSupportedExtensions() const;

    /**
     * @brief Format file size for display
     * @param bytes File size in bytes
     * @return Formatted file size string
     */
    QString formatFileSize(qint64 bytes) const;

    /**
     * @brief Format duration for display
     * @param milliseconds Duration in milliseconds
     * @return Formatted duration string
     */
    QString formatDuration(qint64 milliseconds) const;

    /**
     * @brief Check if file is a supported audio format
     * @param filePath Path to the file
     * @return true if file is supported
     */
    bool isSupportedAudioFile(const QString& filePath) const;

private:
    // Core components
    MusicRepository* m_repository;
    std::unique_ptr<InputValidator> m_validator;
    
    // UI components - File selection
    QGroupBox* m_fileGroup;
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseButton;
    QLabel* m_fileInfoLabel;
    QPushButton* m_extractMetadataButton;
    
    // UI components - Music information
    QGroupBox* m_musicInfoGroup;
    QLineEdit* m_titleEdit;
    QLineEdit* m_artistEdit;
    QLineEdit* m_albumEdit;
    QComboBox* m_genre1Combo;
    QComboBox* m_genre2Combo;
    QComboBox* m_countryCombo;
    QDateEdit* m_dateEdit;
    QTextEdit* m_notesEdit;
    
    // UI components - Validation and status
    QLabel* m_validationStatusLabel;
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    
    // UI components - Buttons
    QPushButton* m_addButton;
    QPushButton* m_cancelButton;
    QPushButton* m_clearButton;
    QPushButton* m_manageGenresButton;
    QPushButton* m_autoFillButton;
    
    // Layouts
    QVBoxLayout* m_mainLayout;
    QFormLayout* m_fileLayout;
    QFormLayout* m_musicInfoLayout;
    QHBoxLayout* m_buttonLayout;
    
    // State management
    QTimer* m_validationTimer;
    QFutureWatcher<QVariantMap>* m_metadataWatcher;
    bool m_isValidating;
    bool m_isExtracting;
    bool m_isAdding;
    int m_addedMusicId;
    
    // Validation state
    QMap<QWidget*, QString> m_fieldErrors;
    bool m_formValid;
    
    // Style sheets for validation
    static const QString VALID_FIELD_STYLE;
    static const QString INVALID_FIELD_STYLE;
    static const QString NEUTRAL_FIELD_STYLE;
};

/**
 * @brief Metadata extraction result structure
 */
struct MetadataExtractionResult
{
    bool success = false;
    QString title;
    QString artist;
    QString album;
    QString genre;
    qint64 duration = 0;
    qint64 fileSize = 0;
    QString format;
    int bitrate = 0;
    int sampleRate = 0;
    QString errorMessage;
    
    /**
     * @brief Convert to QVariantMap for easy handling
     * @return Metadata as QVariantMap
     */
    QVariantMap toVariantMap() const;
    
    /**
     * @brief Create from QVariantMap
     * @param map Metadata as QVariantMap
     * @return MetadataExtractionResult instance
     */
    static MetadataExtractionResult fromVariantMap(const QVariantMap& map);
};

/**
 * @brief Metadata extractor utility class
 */
class MetadataExtractor : public QObject
{
    Q_OBJECT

public:
    explicit MetadataExtractor(QObject* parent = nullptr);
    
    /**
     * @brief Extract metadata from audio file
     * @param filePath Path to the audio file
     * @return Extraction result
     */
    static MetadataExtractionResult extractMetadata(const QString& filePath);
    
    /**
     * @brief Check if metadata extraction tools are available
     * @return true if tools are available
     */
    static bool isMetadataExtractionAvailable();
    
    /**
     * @brief Get list of available metadata extraction tools
     * @return List of available tools
     */
    static QStringList getAvailableTools();

private:
    /**
     * @brief Extract metadata using exiftool
     * @param filePath Path to the audio file
     * @return Extraction result
     */
    static MetadataExtractionResult extractWithExifTool(const QString& filePath);
    
    /**
     * @brief Extract metadata using mediainfo
     * @param filePath Path to the audio file
     * @return Extraction result
     */
    static MetadataExtractionResult extractWithMediaInfo(const QString& filePath);
    
    /**
     * @brief Extract metadata using ffprobe
     * @param filePath Path to the audio file
     * @return Extraction result
     */
    static MetadataExtractionResult extractWithFFProbe(const QString& filePath);
    
    /**
     * @brief Parse duration string to milliseconds
     * @param durationStr Duration string in various formats
     * @return Duration in milliseconds
     */
    static qint64 parseDuration(const QString& durationStr);
    
    /**
     * @brief Parse bitrate string to integer
     * @param bitrateStr Bitrate string
     * @return Bitrate in kbps
     */
    static int parseBitrate(const QString& bitrateStr);
};

#endif // ENHANCEDADDMUSICSINGLEDDIALOG_H