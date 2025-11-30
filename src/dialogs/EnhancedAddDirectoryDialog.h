#ifndef ENHANCEDADDDIRECTORYDIALOG_H
#define ENHANCEDADDDIRECTORYDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QFutureWatcher>
#include <QTreeWidget>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <memory>

// Forward declarations
class MusicRepository;
class InputValidator;
struct MusicItem;
class QLineEdit;
class QComboBox;
class QPushButton;
class QTextEdit;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QSplitter;
class QTreeWidgetItem;

/**
 * @brief Enhanced dialog for importing music directories with progress tracking
 * 
 * This dialog provides a comprehensive interface for importing entire
 * directories of music files with real-time progress tracking, file
 * filtering, and batch processing capabilities.
 * 
 * Features:
 * - Directory tree preview with file filtering
 * - Real-time progress tracking with cancellation
 * - Batch processing with error handling
 * - File type filtering and validation
 * - Duplicate detection and handling
 * - Metadata extraction for all files
 * - Resume capability for interrupted imports
 * 
 * @since XFB 2.0
 */
class EnhancedAddDirectoryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Import options for directory processing
     */
    struct ImportOptions {
        bool includeSubdirectories = true;
        bool extractMetadata = true;
        bool skipDuplicates = true;
        bool overwriteExisting = false;
        QStringList fileExtensions = {"mp3", "ogg", "wav", "flac", "m4a", "aac"};
        QString defaultGenre;
        QString defaultArtist;
        int batchSize = 50;
        bool validateFiles = true;
        bool createBackup = false;
    };

    /**
     * @brief Import statistics
     */
    struct ImportStatistics {
        int totalFiles = 0;
        int processedFiles = 0;
        int successfulImports = 0;
        int skippedFiles = 0;
        int errorFiles = 0;
        qint64 totalSize = 0;
        qint64 processedSize = 0;
        QDateTime startTime;
        QDateTime endTime;
        QStringList errorMessages;
    };

    explicit EnhancedAddDirectoryDialog(MusicRepository* repository, QWidget* parent = nullptr);
    ~EnhancedAddDirectoryDialog();

    /**
     * @brief Set the initial directory path
     * @param directoryPath Path to the directory
     */
    void setDirectoryPath(const QString& directoryPath);

    /**
     * @brief Get import statistics
     * @return Import statistics
     */
    const ImportStatistics& importStatistics() const;

    /**
     * @brief Set import options
     * @param options Import options
     */
    void setImportOptions(const ImportOptions& options);

    /**
     * @brief Get current import options
     * @return Current import options
     */
    ImportOptions importOptions() const;

public slots:
    /**
     * @brief Start the import process
     */
    void startImport();

    /**
     * @brief Cancel the import process
     */
    void cancelImport();

    /**
     * @brief Pause the import process
     */
    void pauseImport();

    /**
     * @brief Resume the import process
     */
    void resumeImport();

signals:
    /**
     * @brief Emitted when import is completed
     * @param statistics Import statistics
     */
    void importCompleted(const ImportStatistics& statistics);

    /**
     * @brief Emitted when import is cancelled
     */
    void importCancelled();

    /**
     * @brief Emitted when import progress changes
     * @param current Current progress
     * @param total Total items
     * @param message Progress message
     */
    void importProgress(int current, int total, const QString& message);

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief Handle directory browse button click
     */
    void onBrowseDirectoryClicked();

    /**
     * @brief Handle directory path change
     */
    void onDirectoryPathChanged();

    /**
     * @brief Handle scan directory button click
     */
    void onScanDirectoryClicked();

    /**
     * @brief Handle directory scan completion
     */
    void onDirectoryScanCompleted();

    /**
     * @brief Handle import button click
     */
    void onImportButtonClicked();

    /**
     * @brief Handle cancel button click
     */
    void onCancelButtonClicked();

    /**
     * @brief Handle pause/resume button click
     */
    void onPauseResumeButtonClicked();

    /**
     * @brief Handle options change
     */
    void onOptionsChanged();

    /**
     * @brief Handle file tree item change
     */
    void onFileTreeItemChanged(QTreeWidgetItem* item, int column);

    /**
     * @brief Handle select all button click
     */
    void onSelectAllClicked();

    /**
     * @brief Handle select none button click
     */
    void onSelectNoneClicked();

    /**
     * @brief Handle import progress update
     */
    void onImportProgressUpdate();

    /**
     * @brief Handle import completion
     */
    void onImportFinished();

    /**
     * @brief Handle progress timer timeout
     */
    void onProgressTimer();

private:
    /**
     * @brief Setup the UI layout and components
     */
    void setupUI();

    /**
     * @brief Setup signal connections
     */
    void setupConnections();

    /**
     * @brief Setup import options UI
     */
    void setupOptionsUI();

    /**
     * @brief Scan directory for audio files
     * @param directoryPath Path to scan
     */
    void scanDirectory(const QString& directoryPath);

    /**
     * @brief Scan directory asynchronously
     * @param directoryPath Path to scan
     * @param options Scan options
     */
    void scanDirectoryAsync(const QString& directoryPath, const ImportOptions& options);

    /**
     * @brief Populate file tree with scan results
     * @param files List of found files
     */
    void populateFileTree(const QStringList& files);

    /**
     * @brief Get selected files from tree
     * @return List of selected file paths
     */
    QStringList getSelectedFiles() const;

    /**
     * @brief Update file tree statistics
     */
    void updateFileTreeStatistics();

    /**
     * @brief Validate import settings
     * @return true if settings are valid
     */
    bool validateImportSettings();

    /**
     * @brief Start import process asynchronously
     * @param files List of files to import
     * @param options Import options
     */
    void startImportAsync(const QStringList& files, const ImportOptions& options);

    /**
     * @brief Update progress display
     * @param current Current progress
     * @param total Total items
     * @param message Progress message
     */
    void updateProgress(int current, int total, const QString& message);

    /**
     * @brief Show import results
     * @param statistics Import statistics
     */
    void showImportResults(const ImportStatistics& statistics);

    /**
     * @brief Enable or disable UI controls
     * @param enabled true to enable controls
     */
    void setControlsEnabled(bool enabled);

    /**
     * @brief Reset dialog to initial state
     */
    void resetDialog();

    /**
     * @brief Format file size for display
     * @param bytes File size in bytes
     * @return Formatted file size string
     */
    QString formatFileSize(qint64 bytes) const;

    /**
     * @brief Format time duration for display
     * @param milliseconds Duration in milliseconds
     * @return Formatted duration string
     */
    QString formatDuration(qint64 milliseconds) const;

    /**
     * @brief Calculate estimated time remaining
     * @return Estimated time in milliseconds
     */
    qint64 calculateEstimatedTime() const;

    /**
     * @brief Create tree item for file
     * @param filePath Path to the file
     * @param parent Parent tree item
     * @return Created tree item
     */
    QTreeWidgetItem* createFileTreeItem(const QString& filePath, QTreeWidgetItem* parent = nullptr);

    /**
     * @brief Update tree item with file info
     * @param item Tree item to update
     * @param filePath Path to the file
     */
    void updateTreeItemInfo(QTreeWidgetItem* item, const QString& filePath);

    /**
     * @brief Check if file should be included based on options
     * @param filePath Path to the file
     * @param options Import options
     * @return true if file should be included
     */
    bool shouldIncludeFile(const QString& filePath, const ImportOptions& options) const;

private:
    // Core components
    MusicRepository* m_repository;
    std::unique_ptr<InputValidator> m_validator;
    
    // UI components - Directory selection
    QGroupBox* m_directoryGroup;
    QLineEdit* m_directoryPathEdit;
    QPushButton* m_browseDirectoryButton;
    QPushButton* m_scanDirectoryButton;
    QLabel* m_directoryInfoLabel;
    
    // UI components - File tree
    QGroupBox* m_fileTreeGroup;
    QTreeWidget* m_fileTreeWidget;
    QLabel* m_fileTreeStatsLabel;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;
    
    // UI components - Import options
    QGroupBox* m_optionsGroup;
    QCheckBox* m_includeSubdirectoriesCheck;
    QCheckBox* m_extractMetadataCheck;
    QCheckBox* m_skipDuplicatesCheck;
    QCheckBox* m_overwriteExistingCheck;
    QCheckBox* m_validateFilesCheck;
    QLineEdit* m_fileExtensionsEdit;
    QLineEdit* m_defaultGenreEdit;
    QLineEdit* m_defaultArtistEdit;
    QLineEdit* m_batchSizeEdit;
    
    // UI components - Progress
    QGroupBox* m_progressGroup;
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    QLabel* m_statisticsLabel;
    QLabel* m_timeLabel;
    
    // UI components - Buttons
    QPushButton* m_importButton;
    QPushButton* m_cancelButton;
    QPushButton* m_pauseResumeButton;
    QPushButton* m_closeButton;
    
    // Layouts
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;
    QHBoxLayout* m_buttonLayout;
    
    // State management
    QTimer* m_progressTimer;
    QFutureWatcher<QStringList>* m_scanWatcher;
    QFutureWatcher<ImportStatistics>* m_importWatcher;
    
    bool m_isScanning;
    bool m_isImporting;
    bool m_isPaused;
    bool m_isCancelled;
    
    // Import data
    ImportOptions m_importOptions;
    ImportStatistics m_importStatistics;
    QStringList m_scannedFiles;
    QStringList m_selectedFiles;
    
    // Progress tracking
    QDateTime m_importStartTime;
    QList<QPair<qint64, int>> m_progressHistory; // timestamp, processed count
    static constexpr int MAX_PROGRESS_HISTORY = 10;
};

/**
 * @brief Directory scanner utility class
 */
class DirectoryScanner : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryScanner(QObject* parent = nullptr);
    
    /**
     * @brief Scan directory for audio files
     * @param directoryPath Path to scan
     * @param options Scan options
     * @return List of found audio files
     */
    static QStringList scanDirectory(const QString& directoryPath, 
                                   const EnhancedAddDirectoryDialog::ImportOptions& options);

private:
    /**
     * @brief Recursively scan directory
     * @param directory Directory to scan
     * @param options Scan options
     * @param files Output list of files
     */
    static void scanDirectoryRecursive(const QDir& directory,
                                     const EnhancedAddDirectoryDialog::ImportOptions& options,
                                     QStringList& files);
    
    /**
     * @brief Check if file is a supported audio file
     * @param filePath Path to the file
     * @param extensions List of supported extensions
     * @return true if file is supported
     */
    static bool isSupportedAudioFile(const QString& filePath, const QStringList& extensions);
};

/**
 * @brief Batch import processor
 */
class BatchImportProcessor : public QObject
{
    Q_OBJECT

public:
    explicit BatchImportProcessor(MusicRepository* repository, QObject* parent = nullptr);
    
    /**
     * @brief Process files in batches
     * @param files List of files to process
     * @param options Import options
     * @return Import statistics
     */
    EnhancedAddDirectoryDialog::ImportStatistics processFiles(
        const QStringList& files,
        const EnhancedAddDirectoryDialog::ImportOptions& options);

signals:
    /**
     * @brief Emitted when progress changes
     * @param current Current progress
     * @param total Total items
     * @param message Progress message
     */
    void progressChanged(int current, int total, const QString& message);

    /**
     * @brief Emitted when processing should be cancelled
     */
    void cancellationRequested();

private:
    /**
     * @brief Process a single file
     * @param filePath Path to the file
     * @param options Import options
     * @return true if file was processed successfully
     */
    bool processSingleFile(const QString& filePath, 
                          const EnhancedAddDirectoryDialog::ImportOptions& options);
    
    /**
     * @brief Check if file already exists in database
     * @param filePath Path to the file
     * @return true if file exists
     */
    bool fileExistsInDatabase(const QString& filePath);
    
    /**
     * @brief Create music item from file
     * @param filePath Path to the file
     * @param options Import options
     * @return Music item
     */
    MusicItem createMusicItemFromFile(const QString& filePath,
                                    const EnhancedAddDirectoryDialog::ImportOptions& options);

private:
    MusicRepository* m_repository;
    std::atomic<bool> m_cancelled;
};

#endif // ENHANCEDADDDIRECTORYDIALOG_H