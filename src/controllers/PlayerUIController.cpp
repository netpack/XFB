#include "PlayerUIController.h"
#include "MainController.h"
#include "../player.h"
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QTimer>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QModelIndex>
#include <QApplication>
#include <QWidget>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

PlayerUIController::PlayerUIController(player* playerWindow, QObject* parent)
    : QObject(parent)
    , m_playerWindow(playerWindow)
    , m_mainController(std::make_unique<MainController>(this))
    , m_uiRefreshTimer(new QTimer(this))
    , m_progressDialog(nullptr)
    , m_initialized(false)
    , m_isPlaying(false)
    , m_currentPosition(0)
    , m_currentDuration(0)
    , m_currentVolume(75)
{
    // Setup UI refresh timer for responsive updates
    m_uiRefreshTimer->setInterval(100); // Update every 100ms
    connect(m_uiRefreshTimer, &QTimer::timeout, this, &PlayerUIController::refreshUI);
}

PlayerUIController::~PlayerUIController()
{
    shutdown();
}

bool PlayerUIController::initialize()
{
    if (m_initialized || !m_playerWindow) {
        return m_initialized;
    }

    qDebug() << "PlayerUIController: Initializing...";

    try {
        // Initialize the main controller
        if (!m_mainController->initialize()) {
            qCritical() << "PlayerUIController: Failed to initialize MainController";
            return false;
        }

        // Set the player window as the main window for the controller
        m_mainController->setMainWindow(m_playerWindow);

        // Setup UI connections
        connectUISignals();
        connectControllerSignals();
        setupProgressIndicators();

        // Initialize UI state
        updateUIState();

        // Start UI refresh timer
        m_uiRefreshTimer->start();

        m_initialized = true;
        qDebug() << "PlayerUIController: Initialization completed successfully";

        return true;

    } catch (const std::exception& e) {
        qCritical() << "PlayerUIController: Initialization failed:" << e.what();
        return false;
    }
}

void PlayerUIController::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "PlayerUIController: Shutting down...";

    m_uiRefreshTimer->stop();
    hideProgressDialog();

    if (m_mainController) {
        m_mainController->shutdown();
    }

    m_initialized = false;
}

MainController* PlayerUIController::mainController() const
{
    return m_mainController.get();
}

void PlayerUIController::onPlayButtonClicked()
{
    if (!m_mainController) {
        return;
    }

    if (m_isPlaying) {
        m_mainController->pauseAudio();
    } else {
        // Get the currently selected track from the music table
        if (m_playerWindow && m_playerWindow->getMusicView()) {
            QModelIndexList selection = m_playerWindow->getMusicView()->selectionModel()->selectedRows();
            if (!selection.isEmpty()) {
                QModelIndex index = selection.first();
                // Assuming column 7 contains the file path (based on the original code)
                QString filePath = index.sibling(index.row(), 7).data().toString();
                if (!filePath.isEmpty()) {
                    m_mainController->playAudio(filePath);
                    m_currentTrack = QFileInfo(filePath).baseName();
                }
            }
        }
    }
}

void PlayerUIController::onPauseButtonClicked()
{
    if (m_mainController) {
        m_mainController->pauseAudio();
    }
}

void PlayerUIController::onStopButtonClicked()
{
    if (m_mainController) {
        m_mainController->stopAudio();
    }
}

void PlayerUIController::onVolumeChanged(int volume)
{
    if (m_mainController) {
        m_mainController->setVolume(volume);
        m_currentVolume = volume;
    }
}

void PlayerUIController::onProgressChanged(int position)
{
    // This would typically seek to the new position
    // For now, we'll just update the current position
    m_currentPosition = position;
}

void PlayerUIController::onMusicSelected(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    // Update UI to show selected track information
    QString title = index.sibling(index.row(), 1).data().toString(); // Assuming column 1 is title
    QString artist = index.sibling(index.row(), 2).data().toString(); // Assuming column 2 is artist
    
    updateStatusMessage(QString("Selected: %1 - %2").arg(artist, title));
}

void PlayerUIController::onImportMusicRequested()
{
    if (!m_mainController) {
        return;
    }

    QStringList filePaths = QFileDialog::getOpenFileNames(
        m_playerWindow,
        tr("Select Music Files"),
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        tr("Audio Files (*.mp3 *.ogg *.wav *.flac *.m4a)")
    );

    if (!filePaths.isEmpty()) {
        showProgressDialog(tr("Importing Music"), tr("Importing selected music files..."));
        m_mainController->importMusic(filePaths);
    }
}

void PlayerUIController::onImportDirectoryRequested()
{
    if (!m_mainController) {
        return;
    }

    QString directoryPath = QFileDialog::getExistingDirectory(
        m_playerWindow,
        tr("Select Music Directory"),
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
    );

    if (!directoryPath.isEmpty()) {
        showProgressDialog(tr("Importing Directory"), tr("Scanning directory for music files..."));
        m_mainController->importDirectory(directoryPath);
    }
}

void PlayerUIController::onSavePlaylistRequested()
{
    if (!m_mainController) {
        return;
    }

    bool ok;
    QString playlistName = QInputDialog::getText(
        m_playerWindow,
        tr("Save Playlist"),
        tr("Enter playlist name:"),
        QLineEdit::Normal,
        QString(),
        &ok
    );

    if (ok && !playlistName.isEmpty()) {
        QList<int> musicIds = getSelectedMusicIds();
        if (!musicIds.isEmpty()) {
            m_mainController->savePlaylist(playlistName, musicIds);
        } else {
            QMessageBox::warning(m_playerWindow, tr("No Selection"), 
                               tr("Please select music tracks to include in the playlist."));
        }
    }
}

void PlayerUIController::onLoadPlaylistRequested()
{
    if (!m_mainController) {
        return;
    }

    // This would typically show a dialog to select from existing playlists
    // For now, we'll use a simple input dialog for the playlist ID
    bool ok;
    int playlistId = QInputDialog::getInt(
        m_playerWindow,
        tr("Load Playlist"),
        tr("Enter playlist ID:"),
        1, 1, 999999, 1, &ok
    );

    if (ok) {
        m_mainController->loadPlaylist(playlistId);
    }
}

void PlayerUIController::onOptimizeDatabaseRequested()
{
    if (!m_mainController) {
        return;
    }

    int ret = QMessageBox::question(
        m_playerWindow,
        tr("Optimize Database"),
        tr("This will optimize the database for better performance. Continue?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (ret == QMessageBox::Yes) {
        showProgressDialog(tr("Database Optimization"), tr("Optimizing database..."));
        m_mainController->optimizeDatabase();
    }
}

void PlayerUIController::updatePlaybackState(bool playing)
{
    m_isPlaying = playing;
    
    if (m_playerWindow) {
        // Update play/pause button state
        if (m_playerWindow->getPlayButton()) {
            m_playerWindow->getPlayButton()->setText(playing ? tr("Pause") : tr("Play"));
        }
        
        // Update status
        QString status = playing ? tr("Playing") : tr("Stopped");
        if (!m_currentTrack.isEmpty()) {
            status += QString(": %1").arg(m_currentTrack);
        }
        updateStatusMessage(status);
    }
}

void PlayerUIController::updatePosition(qint64 position)
{
    m_currentPosition = position;
    
    if (m_playerWindow && m_playerWindow->getProgressSlider()) {
        // Update progress slider without triggering signals
        m_playerWindow->getProgressSlider()->blockSignals(true);
        if (m_currentDuration > 0) {
            int sliderPosition = static_cast<int>((position * 100) / m_currentDuration);
            m_playerWindow->getProgressSlider()->setValue(sliderPosition);
        }
        m_playerWindow->getProgressSlider()->blockSignals(false);
    }
}

void PlayerUIController::updateDuration(qint64 duration)
{
    m_currentDuration = duration;
    
    // Update UI with duration information if needed
    if (m_playerWindow && m_playerWindow->ui) {
        // Format and display duration
        int seconds = static_cast<int>(duration / 1000);
        int minutes = seconds / 60;
        seconds = seconds % 60;
        QString durationText = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        // Update duration label if it exists
        // This would depend on the specific UI layout
    }
}

void PlayerUIController::updateVolume(int volume)
{
    m_currentVolume = volume;
    
    if (m_playerWindow && m_playerWindow->getVolumeSlider()) {
        // Update volume slider without triggering signals
        m_playerWindow->getVolumeSlider()->blockSignals(true);
        m_playerWindow->getVolumeSlider()->setValue(volume);
        m_playerWindow->getVolumeSlider()->blockSignals(false);
    }
}

void PlayerUIController::showError(const QString& title, const QString& message)
{
    if (m_playerWindow) {
        QMessageBox::critical(m_playerWindow, title, message);
    }
}

void PlayerUIController::updateStatusMessage(const QString& message)
{
    if (m_playerWindow && m_playerWindow->statusBar()) {
        m_playerWindow->statusBar()->showMessage(message);
    }
}

void PlayerUIController::onMusicImportCompleted(bool success, int importedCount)
{
    hideProgressDialog();
    
    if (success) {
        QString message = tr("Successfully imported %1 music files").arg(importedCount);
        updateStatusMessage(message);
        refreshMusicTable();
        
        if (importedCount > 0) {
            QMessageBox::information(m_playerWindow, tr("Import Complete"), message);
        }
    } else {
        showError(tr("Import Failed"), tr("Failed to import music files"));
    }
}

void PlayerUIController::onPlaylistOperationCompleted(bool success, const QString& message)
{
    hideProgressDialog();
    
    if (success) {
        updateStatusMessage(message);
        QMessageBox::information(m_playerWindow, tr("Playlist Operation"), message);
    } else {
        showError(tr("Playlist Error"), message);
    }
}

void PlayerUIController::connectUISignals()
{
    if (!m_playerWindow || !m_playerWindow->ui) {
        return;
    }

    // Connect play/pause/stop buttons
    if (m_playerWindow->getPlayButton()) {
        connect(m_playerWindow->getPlayButton(), &QPushButton::clicked,
                this, &PlayerUIController::onPlayButtonClicked);
    }
    
    if (m_playerWindow->getStopButton()) {
        connect(m_playerWindow->getStopButton(), &QPushButton::clicked,
                this, &PlayerUIController::onStopButtonClicked);
    }

    // Connect volume slider
    if (m_playerWindow->getVolumeSlider()) {
        connect(m_playerWindow->getVolumeSlider(), &QSlider::valueChanged,
                this, &PlayerUIController::onVolumeChanged);
    }

    // Connect progress slider
    if (m_playerWindow->getProgressSlider()) {
        connect(m_playerWindow->getProgressSlider(), &QSlider::sliderMoved,
                this, &PlayerUIController::onProgressChanged);
    }

    // Connect music table selection
    if (m_playerWindow->getMusicView() && m_playerWindow->getMusicView()->selectionModel()) {
        connect(m_playerWindow->getMusicView()->selectionModel(), 
                &QItemSelectionModel::currentRowChanged,
                this, &PlayerUIController::onMusicSelected);
    }

    // Connect menu actions (these would need to be connected to the appropriate menu items)
    // This would be done based on the specific menu structure in the UI
}

void PlayerUIController::connectControllerSignals()
{
    if (!m_mainController) {
        return;
    }

    // Connect controller signals to UI update slots
    connect(m_mainController.get(), &MainController::playbackStateChanged,
            this, &PlayerUIController::updatePlaybackState);
    
    connect(m_mainController.get(), &MainController::positionChanged,
            this, &PlayerUIController::updatePosition);
    
    connect(m_mainController.get(), &MainController::durationChanged,
            this, &PlayerUIController::updateDuration);
    
    connect(m_mainController.get(), &MainController::volumeChanged,
            this, &PlayerUIController::updateVolume);
    
    connect(m_mainController.get(), &MainController::errorOccurred,
            this, &PlayerUIController::showError);
    
    connect(m_mainController.get(), &MainController::statusMessageChanged,
            this, &PlayerUIController::updateStatusMessage);
    
    connect(m_mainController.get(), &MainController::musicImportCompleted,
            this, &PlayerUIController::onMusicImportCompleted);
    
    connect(m_mainController.get(), &MainController::playlistOperationCompleted,
            this, &PlayerUIController::onPlaylistOperationCompleted);
}

void PlayerUIController::setupProgressIndicators()
{
    // Setup any additional progress indicators if needed
    // This could include progress bars in the status bar, etc.
}

void PlayerUIController::updateUIState()
{
    // Update UI elements to reflect current state
    updatePlaybackState(m_isPlaying);
    updateVolume(m_currentVolume);
    updatePosition(m_currentPosition);
    updateDuration(m_currentDuration);
}

QList<int> PlayerUIController::getSelectedMusicIds() const
{
    QList<int> musicIds;
    
    if (m_playerWindow && m_playerWindow->getMusicView()) {
        QModelIndexList selection = m_playerWindow->getMusicView()->selectionModel()->selectedRows();
        for (const QModelIndex& index : selection) {
            // Assuming column 0 contains the music ID
            bool ok;
            int id = index.sibling(index.row(), 0).data().toInt(&ok);
            if (ok) {
                musicIds.append(id);
            }
        }
    }
    
    return musicIds;
}

void PlayerUIController::refreshMusicTable()
{
    if (m_playerWindow && m_playerWindow->getMusicView()) {
        // Refresh the music table model
        if (auto* model = m_playerWindow->getMusicView()->model()) {
            // This would depend on the specific model implementation
            // For QSqlTableModel, we would call select()
            if (auto* sqlModel = qobject_cast<QSqlTableModel*>(model)) {
                sqlModel->select();
            }
        }
    }
}

void PlayerUIController::showProgressDialog(const QString& title, const QString& message)
{
    hideProgressDialog();
    
    m_progressDialog = new QProgressDialog(m_playerWindow);
    m_progressDialog->setWindowTitle(title);
    m_progressDialog->setLabelText(message);
    m_progressDialog->setRange(0, 0); // Indeterminate progress
    m_progressDialog->setModal(true);
    m_progressDialog->show();
}

void PlayerUIController::hideProgressDialog()
{
    if (m_progressDialog) {
        m_progressDialog->hide();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
}

void PlayerUIController::updateProgressIndicators()
{
    // Update any progress indicators
    QApplication::processEvents();
}

void PlayerUIController::refreshUI()
{
    // Periodic UI refresh for responsive updates
    if (m_initialized) {
        updateProgressIndicators();
    }
}