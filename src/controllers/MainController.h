#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QProgressDialog>
#include <QTimer>
#include <memory>

// Forward declarations
class AudioService;
class DatabaseService;
class ConfigurationService;
class AccessibilityManager;
class MusicRepository;
class PlaylistRepository;
class GenreRepository;
class ServiceContainer;
class QWidget; // Use QWidget instead of player to avoid circular dependency

/**
 * @brief Main controller that coordinates between UI and business logic
 * 
 * The MainController implements the controller pattern to separate UI concerns
 * from business logic. It manages service dependencies and coordinates operations
 * between the UI layer and service layer.
 * 
 * @since XFB 2.0
 */
class MainController : public QObject
{
    Q_OBJECT

public:
    explicit MainController(QObject* parent = nullptr);
    ~MainController();

    /**
     * @brief Initialize the controller and all services
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the controller and cleanup resources
     */
    void shutdown();

    /**
     * @brief Set the main UI window reference
     * @param mainWindow Pointer to the main UI window
     */
    void setMainWindow(QWidget* mainWindow);

    // Service accessors
    AudioService* audioService() const;
    DatabaseService* databaseService() const;
    ConfigurationService* configurationService() const;
    AccessibilityManager* accessibilityManager() const;
    MusicRepository* musicRepository() const;
    PlaylistRepository* playlistRepository() const;
    GenreRepository* genreRepository() const;

    /**
     * @brief Show progress dialog for long-running operations
     * @param title Dialog title
     * @param message Initial message
     * @param maximum Maximum progress value (0 for indeterminate)
     * @return Pointer to the progress dialog
     */
    QProgressDialog* showProgressDialog(const QString& title, 
                                       const QString& message, 
                                       int maximum = 0);

    /**
     * @brief Hide the current progress dialog
     */
    void hideProgressDialog();

    /**
     * @brief Update progress dialog with new values
     * @param value Current progress value
     * @param message Optional new message
     */
    void updateProgress(int value, const QString& message = QString());

public slots:
    /**
     * @brief Handle audio playback requests
     * @param filePath Path to the audio file
     */
    void playAudio(const QString& filePath);

    /**
     * @brief Handle audio pause requests
     */
    void pauseAudio();

    /**
     * @brief Handle audio stop requests
     */
    void stopAudio();

    /**
     * @brief Handle volume change requests
     * @param volume Volume level (0-100)
     */
    void setVolume(int volume);

    /**
     * @brief Handle music import requests
     * @param filePaths List of file paths to import
     */
    void importMusic(const QStringList& filePaths);

    /**
     * @brief Handle directory import requests
     * @param directoryPath Path to directory to scan
     */
    void importDirectory(const QString& directoryPath);

    /**
     * @brief Handle playlist save requests
     * @param playlistName Name of the playlist
     * @param musicIds List of music IDs in the playlist
     */
    void savePlaylist(const QString& playlistName, const QList<int>& musicIds);

    /**
     * @brief Handle playlist load requests
     * @param playlistId ID of the playlist to load
     */
    void loadPlaylist(int playlistId);

    /**
     * @brief Handle database optimization requests
     */
    void optimizeDatabase();

signals:
    /**
     * @brief Emitted when audio playback state changes
     * @param playing true if playing, false if stopped/paused
     */
    void playbackStateChanged(bool playing);

    /**
     * @brief Emitted when audio position changes
     * @param position Current position in milliseconds
     */
    void positionChanged(qint64 position);

    /**
     * @brief Emitted when audio duration is available
     * @param duration Total duration in milliseconds
     */
    void durationChanged(qint64 duration);

    /**
     * @brief Emitted when volume changes
     * @param volume Current volume level (0-100)
     */
    void volumeChanged(int volume);

    /**
     * @brief Emitted when music import is completed
     * @param success true if import was successful
     * @param importedCount Number of files imported
     */
    void musicImportCompleted(bool success, int importedCount);

    /**
     * @brief Emitted when playlist operation is completed
     * @param success true if operation was successful
     * @param message Result message
     */
    void playlistOperationCompleted(bool success, const QString& message);

    /**
     * @brief Emitted when an error occurs
     * @param title Error title
     * @param message Error message
     */
    void errorOccurred(const QString& title, const QString& message);

    /**
     * @brief Emitted when a status message should be displayed
     * @param message Status message
     */
    void statusMessageChanged(const QString& message);

private slots:
    /**
     * @brief Handle service errors
     * @param serviceName Name of the service that had an error
     * @param error Error message
     */
    void handleServiceError(const QString& serviceName, const QString& error);

    /**
     * @brief Handle long-running operation progress
     * @param current Current progress value
     * @param total Total progress value
     * @param message Progress message
     */
    void handleOperationProgress(int current, int total, const QString& message);

private:
    /**
     * @brief Setup all services and dependencies
     */
    void setupServices();

    /**
     * @brief Connect signals between services and controller
     */
    void connectServiceSignals();

    /**
     * @brief Connect signals between controller and UI
     */
    void connectUISignals();

    /**
     * @brief Validate that all required services are available
     * @return true if all services are valid
     */
    bool validateServices() const;

private:
    ServiceContainer* m_serviceContainer;
    std::unique_ptr<MusicRepository> m_musicRepository;
    std::unique_ptr<PlaylistRepository> m_playlistRepository;
    std::unique_ptr<GenreRepository> m_genreRepository;
    QWidget* m_mainWindow;
    QProgressDialog* m_progressDialog;
    QTimer* m_progressTimer;
    bool m_initialized;
};

#endif // MAINCONTROLLER_H