#ifndef PLAYERUICONTROLLER_H
#define PLAYERUICONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QProgressDialog>
#include <QtMultimedia/QMediaPlayer>
#include <memory>

// Forward declarations
class MainController;
class player; // The existing UI class
class QSlider;
class QLabel;
class QPushButton;
class QTableView;

/**
 * @brief UI Controller for the main player window
 * 
 * This controller acts as an intermediary between the player UI and the MainController,
 * implementing the controller pattern to separate UI concerns from business logic.
 * It handles UI events and delegates business operations to the MainController.
 * 
 * @since XFB 2.0
 */
class PlayerUIController : public QObject
{
    Q_OBJECT

public:
    explicit PlayerUIController(player* playerWindow, QObject* parent = nullptr);
    ~PlayerUIController();

    /**
     * @brief Initialize the UI controller
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the UI controller
     */
    void shutdown();

    /**
     * @brief Get the main controller instance
     * @return Pointer to the main controller
     */
    MainController* mainController() const;

public slots:
    /**
     * @brief Handle play button click
     */
    void onPlayButtonClicked();

    /**
     * @brief Handle pause button click
     */
    void onPauseButtonClicked();

    /**
     * @brief Handle stop button click
     */
    void onStopButtonClicked();

    /**
     * @brief Handle volume slider changes
     * @param volume New volume level (0-100)
     */
    void onVolumeChanged(int volume);

    /**
     * @brief Handle progress slider changes
     * @param position New position in milliseconds
     */
    void onProgressChanged(int position);

    /**
     * @brief Handle music table selection
     * @param index Selected model index
     */
    void onMusicSelected(const QModelIndex& index);

    /**
     * @brief Handle music import request
     */
    void onImportMusicRequested();

    /**
     * @brief Handle directory import request
     */
    void onImportDirectoryRequested();

    /**
     * @brief Handle playlist save request
     */
    void onSavePlaylistRequested();

    /**
     * @brief Handle playlist load request
     */
    void onLoadPlaylistRequested();

    /**
     * @brief Handle database optimization request
     */
    void onOptimizeDatabaseRequested();

    /**
     * @brief Update UI with current playback state
     * @param playing true if playing, false otherwise
     */
    void updatePlaybackState(bool playing);

    /**
     * @brief Update UI with current position
     * @param position Current position in milliseconds
     */
    void updatePosition(qint64 position);

    /**
     * @brief Update UI with current duration
     * @param duration Total duration in milliseconds
     */
    void updateDuration(qint64 duration);

    /**
     * @brief Update UI with current volume
     * @param volume Current volume level (0-100)
     */
    void updateVolume(int volume);

    /**
     * @brief Show error message to user
     * @param title Error title
     * @param message Error message
     */
    void showError(const QString& title, const QString& message);

    /**
     * @brief Update status bar message
     * @param message Status message
     */
    void updateStatusMessage(const QString& message);

    /**
     * @brief Handle music import completion
     * @param success true if import was successful
     * @param importedCount Number of files imported
     */
    void onMusicImportCompleted(bool success, int importedCount);

    /**
     * @brief Handle playlist operation completion
     * @param success true if operation was successful
     * @param message Result message
     */
    void onPlaylistOperationCompleted(bool success, const QString& message);

private slots:
    /**
     * @brief Update progress indicators for long-running operations
     */
    void updateProgressIndicators();

    /**
     * @brief Handle UI refresh timer
     */
    void refreshUI();

private:
    /**
     * @brief Connect UI signals to controller slots
     */
    void connectUISignals();

    /**
     * @brief Connect controller signals to UI slots
     */
    void connectControllerSignals();

    /**
     * @brief Setup progress indicators
     */
    void setupProgressIndicators();

    /**
     * @brief Update UI elements based on current state
     */
    void updateUIState();

    /**
     * @brief Get selected music IDs from the music table
     * @return List of selected music IDs
     */
    QList<int> getSelectedMusicIds() const;

    /**
     * @brief Refresh music table view
     */
    void refreshMusicTable();

    /**
     * @brief Show progress dialog for long operations
     * @param title Dialog title
     * @param message Initial message
     */
    void showProgressDialog(const QString& title, const QString& message);

    /**
     * @brief Hide progress dialog
     */
    void hideProgressDialog();

private:
    player* m_playerWindow;
    std::unique_ptr<MainController> m_mainController;
    QTimer* m_uiRefreshTimer;
    QProgressDialog* m_progressDialog;
    bool m_initialized;
    bool m_isPlaying;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    int m_currentVolume;
    QString m_currentTrack;
};

#endif // PLAYERUICONTROLLER_H