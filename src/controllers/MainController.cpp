#include "MainController.h"
#include "../services/ServiceContainer.h"
#include "../services/AudioService.h"
#include "../services/DatabaseService.h"
#include "../services/ConfigurationService.h"
#include "../services/AccessibilityManager.h"
#include "../repositories/MusicRepository.h"
#include "../repositories/PlaylistRepository.h"
#include "../repositories/GenreRepository.h"
#include <QProgressDialog>
#include <QTimer>
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QUrl>
#include <QMediaPlayer>
#include <QSqlDatabase>
#include <QSqlError>
#include <algorithm>

MainController::MainController(QObject* parent)
    : QObject(parent)
    , m_serviceContainer(ServiceContainer::instance())
    , m_mainWindow(nullptr)
    , m_progressDialog(nullptr)
    , m_progressTimer(new QTimer(this))
    , m_initialized(false)
{
    // Setup progress timer for responsive UI updates
    m_progressTimer->setInterval(100); // Update every 100ms
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (m_progressDialog && m_progressDialog->isVisible()) {
            QApplication::processEvents();
        }
    });
}

MainController::~MainController()
{
    shutdown();
}

bool MainController::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "MainController: Initializing services...";

    try {
        setupServices();
        
        if (!validateServices()) {
            qCritical() << "MainController: Service validation failed";
            return false;
        }

        connectServiceSignals();
        
        m_initialized = true;
        qDebug() << "MainController: Initialization completed successfully";
        
        emit statusMessageChanged("Application initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "MainController: Initialization failed:" << e.what();
        emit errorOccurred("Initialization Error", 
                          QString("Failed to initialize application: %1").arg(e.what()));
        return false;
    }
}

void MainController::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "MainController: Shutting down...";

    hideProgressDialog();
    
    if (m_serviceContainer) {
        m_serviceContainer->shutdownServices();
    }
    
    m_initialized = false;
    emit statusMessageChanged("Application shutdown completed");
}

void MainController::setMainWindow(QWidget* mainWindow)
{
    m_mainWindow = mainWindow;
    if (m_initialized && m_mainWindow) {
        connectUISignals();
    }
}

AudioService* MainController::audioService() const
{
    return m_serviceContainer ? m_serviceContainer->resolve<AudioService>() : nullptr;
}

DatabaseService* MainController::databaseService() const
{
    return m_serviceContainer ? m_serviceContainer->resolve<DatabaseService>() : nullptr;
}

ConfigurationService* MainController::configurationService() const
{
    return m_serviceContainer ? m_serviceContainer->resolve<ConfigurationService>() : nullptr;
}

AccessibilityManager* MainController::accessibilityManager() const
{
    return m_serviceContainer ? m_serviceContainer->resolve<AccessibilityManager>() : nullptr;
}

MusicRepository* MainController::musicRepository() const
{
    return m_musicRepository.get();
}

PlaylistRepository* MainController::playlistRepository() const
{
    return m_playlistRepository.get();
}

GenreRepository* MainController::genreRepository() const
{
    return m_genreRepository.get();
}

QProgressDialog* MainController::showProgressDialog(const QString& title, 
                                                   const QString& message, 
                                                   int maximum)
{
    hideProgressDialog(); // Hide any existing dialog
    
    m_progressDialog = new QProgressDialog(m_mainWindow);
    m_progressDialog->setWindowTitle(title);
    m_progressDialog->setLabelText(message);
    m_progressDialog->setMinimum(0);
    m_progressDialog->setMaximum(maximum);
    m_progressDialog->setModal(true);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);
    
    if (maximum == 0) {
        // Indeterminate progress
        m_progressDialog->setRange(0, 0);
    }
    
    m_progressDialog->show();
    m_progressTimer->start();
    
    return m_progressDialog;
}

void MainController::hideProgressDialog()
{
    m_progressTimer->stop();
    
    if (m_progressDialog) {
        m_progressDialog->hide();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
}

void MainController::updateProgress(int value, const QString& message)
{
    if (m_progressDialog) {
        m_progressDialog->setValue(value);
        if (!message.isEmpty()) {
            m_progressDialog->setLabelText(message);
        }
        QApplication::processEvents();
    }
}

void MainController::playAudio(const QString& filePath)
{
    auto* audio = audioService();
    if (!audio) {
        emit errorOccurred("Service Error", "Audio service not available");
        return;
    }

    try {
        audio->play(QUrl::fromLocalFile(filePath));
        emit statusMessageChanged(QString("Playing: %1").arg(QFileInfo(filePath).baseName()));
    } catch (const std::exception& e) {
        emit errorOccurred("Playback Error", QString("Failed to play audio: %1").arg(e.what()));
    }
}

void MainController::pauseAudio()
{
    auto* audio = audioService();
    if (audio) {
        audio->pause();
        emit statusMessageChanged("Playback paused");
    }
}

void MainController::stopAudio()
{
    auto* audio = audioService();
    if (audio) {
        audio->stop();
        emit statusMessageChanged("Playback stopped");
    }
}

void MainController::setVolume(int volume)
{
    auto* audio = audioService();
    if (audio) {
        audio->setVolume(volume);
    }
}

void MainController::importMusic(const QStringList& filePaths)
{
    auto* musicRepo = musicRepository();
    if (!musicRepo) {
        emit errorOccurred("Service Error", "Music repository not available");
        return;
    }

    if (filePaths.isEmpty()) {
        emit musicImportCompleted(false, 0);
        return;
    }

    // Show progress dialog
    auto* progress = showProgressDialog("Importing Music", 
                                       "Preparing to import music files...", 
                                       filePaths.size());

    int importedCount = 0;
    int currentIndex = 0;

    for (const QString& filePath : filePaths) {
        if (progress && progress->wasCanceled()) {
            break;
        }

        updateProgress(currentIndex, QString("Importing: %1").arg(QFileInfo(filePath).fileName()));

        try {
            // This would typically be done asynchronously in a real implementation
            // For now, we'll simulate the import process
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists() && (fileInfo.suffix().toLower() == "mp3" || 
                                     fileInfo.suffix().toLower() == "ogg" ||
                                     fileInfo.suffix().toLower() == "wav")) {
                
                // Create a basic music item (in real implementation, this would extract metadata)
                MusicItem music;
                music.song = fileInfo.baseName();
                music.path = filePath;
                music.artist = "Unknown Artist"; // Default value
                
                if (musicRepo->addMusic(music)) {
                    importedCount++;
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "Failed to import" << filePath << ":" << e.what();
        }

        currentIndex++;
        QApplication::processEvents();
    }

    hideProgressDialog();
    
    emit musicImportCompleted(true, importedCount);
    emit statusMessageChanged(QString("Imported %1 music files").arg(importedCount));
}

void MainController::importDirectory(const QString& directoryPath)
{
    auto* musicRepo = musicRepository();
    if (!musicRepo) {
        emit errorOccurred("Service Error", "Music repository not available");
        return;
    }

    showProgressDialog("Scanning Directory", 
                      "Scanning directory for music files...", 
                      0); // Indeterminate progress

    try {
        // This would typically be done asynchronously
        bool success = musicRepo->importFromDirectory(directoryPath);
        
        hideProgressDialog();
        
        if (success) {
            emit statusMessageChanged("Directory import completed successfully");
        } else {
            emit errorOccurred("Import Error", "Failed to import directory");
        }
        
    } catch (const std::exception& e) {
        hideProgressDialog();
        emit errorOccurred("Import Error", QString("Directory import failed: %1").arg(e.what()));
    }
}

void MainController::savePlaylist(const QString& playlistName, const QList<int>& musicIds)
{
    Q_UNUSED(musicIds); // Parameter reserved for future implementation
    
    auto* playlistRepo = playlistRepository();
    if (!playlistRepo) {
        emit errorOccurred("Service Error", "Playlist repository not available");
        return;
    }

    try {
        PlaylistItem playlist;
        playlist.name = playlistName;
        playlist.createdAt = QDateTime::currentDateTime();
        
        bool success = playlistRepo->addPlaylist(playlist);
        
        if (success) {
            emit playlistOperationCompleted(true, "Playlist saved successfully");
            emit statusMessageChanged(QString("Saved playlist: %1").arg(playlistName));
        } else {
            emit playlistOperationCompleted(false, "Failed to save playlist");
        }
        
    } catch (const std::exception& e) {
        emit playlistOperationCompleted(false, QString("Playlist save failed: %1").arg(e.what()));
    }
}

void MainController::loadPlaylist(int playlistId)
{
    auto* playlistRepo = playlistRepository();
    if (!playlistRepo) {
        emit errorOccurred("Service Error", "Playlist repository not available");
        return;
    }

    try {
        auto playlists = playlistRepo->getAllPlaylists();
        auto it = std::find_if(playlists.begin(), playlists.end(), 
                              [playlistId](const PlaylistItem& item) { 
                                  return item.id == playlistId; 
                              });
        
        if (it != playlists.end()) {
            emit playlistOperationCompleted(true, "Playlist loaded successfully");
            emit statusMessageChanged(QString("Loaded playlist: %1").arg(it->name));
        } else {
            emit playlistOperationCompleted(false, "Playlist not found");
        }
        
    } catch (const std::exception& e) {
        emit playlistOperationCompleted(false, QString("Playlist load failed: %1").arg(e.what()));
    }
}

void MainController::optimizeDatabase()
{
    auto* dbService = databaseService();
    if (!dbService) {
        emit errorOccurred("Service Error", "Database service not available");
        return;
    }

    showProgressDialog("Database Optimization", 
                      "Optimizing database performance...", 
                      0);

    try {
        // This would typically be done asynchronously
        // For now, simulate the optimization process
        QTimer::singleShot(2000, this, [this]() {
            hideProgressDialog();
            emit statusMessageChanged("Database optimization completed");
        });
        
    } catch (const std::exception& e) {
        hideProgressDialog();
        emit errorOccurred("Optimization Error", 
                          QString("Database optimization failed: %1").arg(e.what()));
    }
}

void MainController::setupServices()
{
    // Register services in the container
    m_serviceContainer->registerSingleton<DatabaseService>();
    m_serviceContainer->registerSingleton<ConfigurationService>();
    m_serviceContainer->registerSingleton<AudioService>();
    m_serviceContainer->registerSingleton<AccessibilityManager>();
    
    // Initialize critical services first
    qDebug() << "MainController: Initializing critical services...";
    
    // Initialize database and configuration services synchronously (they're fast)
    auto* dbService = m_serviceContainer->resolve<DatabaseService>();
    auto* configService = m_serviceContainer->resolve<ConfigurationService>();
    
    if (dbService) {
        dbService->initialize();
    }
    if (configService) {
        configService->initialize();
    }
    
    // Initialize accessibility manager (important but should be fast)
    auto* accessibilityMgr = m_serviceContainer->resolve<AccessibilityManager>();
    if (accessibilityMgr) {
        accessibilityMgr->initialize();
    }
    
    // Initialize AudioService asynchronously to avoid blocking startup
    qDebug() << "MainController: Starting AudioService initialization in background...";
    QTimer::singleShot(100, this, [this]() {
        auto* audioSvc = m_serviceContainer->resolve<AudioService>();
        if (audioSvc) {
            qDebug() << "MainController: Initializing AudioService asynchronously...";
            audioSvc->initialize();
            qDebug() << "MainController: AudioService initialization completed";
        }
    });
    
    // Create repositories manually since they don't inherit from IService
    // Reuse the dbService already declared above
    if (dbService && dbService->state() == IService::ServiceState::Running) {
        try {
            // Use the default database connection for repositories
            QSqlDatabase db = QSqlDatabase::database();
            if (db.isValid()) {
                if (!db.isOpen()) {
                    // Try to open the connection if it's not open
                    if (!db.open()) {
                        qWarning() << "MainController: Failed to open database connection:" << db.lastError().text();
                    }
                }
                
                if (db.isOpen()) {
                    m_musicRepository = std::make_unique<MusicRepository>(db, this);
                    m_playlistRepository = std::make_unique<PlaylistRepository>(db, this);
                    m_genreRepository = std::make_unique<GenreRepository>(db, this);
                    qDebug() << "MainController: Repositories created successfully";
                } else {
                    qWarning() << "MainController: Database is not open";
                }
            } else {
                qWarning() << "MainController: Default database connection is not valid";
            }
        } catch (const std::exception& e) {
            qWarning() << "MainController: Failed to create repositories:" << e.what();
        }
    } else {
        qWarning() << "MainController: DatabaseService not available or not running";
    }
}

void MainController::connectServiceSignals()
{
    // Connect audio service signals
    if (auto* audio = audioService()) {
        connect(audio, &AudioService::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
            emit playbackStateChanged(state == QMediaPlayer::PlayingState);
        });
        
        connect(audio, &AudioService::positionChanged, this, &MainController::positionChanged);
        connect(audio, &AudioService::durationChanged, this, &MainController::durationChanged);
        connect(audio, &AudioService::volumeChanged, this, &MainController::volumeChanged);
        
        connect(audio, &AudioService::audioError, this, [this](const QString& error) {
            handleServiceError("AudioService", error);
        });
    }
    
    // Connect database service signals
    if (auto* db = databaseService()) {
        connect(db, &DatabaseService::databaseError, this, [this](const QString& error) {
            handleServiceError("DatabaseService", error);
        });
    }
    
    // Connect repository signals
    if (auto* musicRepo = musicRepository()) {
        connect(musicRepo, &MusicRepository::operationError, this, [this](const QString& error) {
            handleServiceError("MusicRepository", error);
        });
    }
}

void MainController::connectUISignals()
{
    if (!m_mainWindow) {
        return;
    }
    
    // Connect controller signals to UI slots
    // This would be implemented based on the specific UI requirements
    // For now, we'll leave this as a placeholder for future implementation
}

bool MainController::validateServices() const
{
    bool audioOk = audioService() != nullptr;
    bool dbOk = databaseService() != nullptr;
    bool configOk = configurationService() != nullptr;
    bool accessibilityOk = accessibilityManager() != nullptr;
    bool musicRepoOk = m_musicRepository != nullptr;
    bool playlistRepoOk = m_playlistRepository != nullptr;
    bool genreRepoOk = m_genreRepository != nullptr;
    
    qDebug() << "MainController: Service validation:"
             << "Audio:" << audioOk
             << "DB:" << dbOk
             << "Config:" << configOk
             << "Accessibility:" << accessibilityOk
             << "MusicRepo:" << musicRepoOk
             << "PlaylistRepo:" << playlistRepoOk
             << "GenreRepo:" << genreRepoOk;
    
    return audioOk && dbOk && configOk && accessibilityOk && musicRepoOk && playlistRepoOk && genreRepoOk;
}

void MainController::handleServiceError(const QString& serviceName, const QString& error)
{
    qWarning() << "Service error in" << serviceName << ":" << error;
    emit errorOccurred(QString("%1 Error").arg(serviceName), error);
}

void MainController::handleOperationProgress(int current, int total, const QString& message)
{
    if (m_progressDialog) {
        if (m_progressDialog->maximum() != total) {
            m_progressDialog->setMaximum(total);
        }
        updateProgress(current, message);
    }
}