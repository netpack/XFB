/**
 * @file PlayerIntegration.cpp
 * @brief Example integration of PlayerUIController with existing player class
 * 
 * This file demonstrates how to integrate the new controller pattern
 * with the existing player UI class. It shows the minimal changes
 * needed to adopt the controller pattern while maintaining compatibility.
 * 
 * @since XFB 2.0
 */

#include "player.h"
#include "controllers/PlayerUIController.h"
#include "controllers/ModernSignalConnections.h"
#include "ui/ProgressIndicatorWidget.h"
#include <QVBoxLayout>
#include <QDebug>

/**
 * @brief Enhanced player class with controller pattern integration
 * 
 * This class extends the existing player class to demonstrate
 * how to integrate the new controller pattern. In a real implementation,
 * these changes would be made directly to the player class.
 */
class EnhancedPlayer : public player
{
    Q_OBJECT

public:
    explicit EnhancedPlayer(QWidget* parent = nullptr)
        : player(parent)
        , m_uiController(nullptr)
        , m_progressIndicator(nullptr)
        , m_connectionManager(std::make_unique<ConnectionManager>())
    {
        setupControllerIntegration();
    }

    ~EnhancedPlayer()
    {
        if (m_uiController) {
            m_uiController->shutdown();
        }
    }

    /**
     * @brief Get the UI controller instance
     * @return Pointer to the UI controller
     */
    PlayerUIController* uiController() const
    {
        return m_uiController.get();
    }

    /**
     * @brief Get the progress indicator widget
     * @return Pointer to the progress indicator
     */
    ProgressIndicatorWidget* progressIndicator() const
    {
        return m_progressIndicator;
    }

protected:
    /**
     * @brief Override closeEvent to ensure proper shutdown
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override
    {
        if (m_uiController) {
            m_uiController->shutdown();
        }
        player::closeEvent(event);
    }

private slots:
    /**
     * @brief Handle controller initialization completion
     */
    void onControllerInitialized()
    {
        qDebug() << "EnhancedPlayer: Controller initialized successfully";
        
        // Connect additional UI elements that weren't handled automatically
        connectAdditionalUIElements();
        
        // Setup progress indicator
        setupProgressIndicator();
        
        // Enable UI elements that depend on the controller
        enableControllerDependentUI();
    }

    /**
     * @brief Handle controller errors
     * @param title Error title
     * @param message Error message
     */
    void onControllerError(const QString& title, const QString& message)
    {
        qWarning() << "EnhancedPlayer: Controller error:" << title << "-" << message;
        
        // Show error to user and potentially disable affected functionality
        statusBar()->showMessage(QString("Error: %1").arg(message), 5000);
    }

    /**
     * @brief Handle long-running operations
     * @param title Operation title
     * @param message Operation message
     */
    void onLongOperationStarted(const QString& title, const QString& message)
    {
        if (m_progressIndicator) {
            m_progressIndicator->showIndeterminateProgress(title, message);
        }
    }

    /**
     * @brief Handle operation completion
     */
    void onLongOperationCompleted()
    {
        if (m_progressIndicator) {
            m_progressIndicator->hideProgress();
        }
    }

private:
    /**
     * @brief Setup the controller integration
     */
    void setupControllerIntegration()
    {
        qDebug() << "EnhancedPlayer: Setting up controller integration...";
        
        // Create the UI controller
        m_uiController = std::make_unique<PlayerUIController>(this);
        
        // Connect controller signals using modern syntax
        setupControllerConnections();
        
        // Initialize the controller asynchronously to avoid blocking the UI
        QTimer::singleShot(0, this, [this]() {
            if (m_uiController->initialize()) {
                onControllerInitialized();
            } else {
                onControllerError("Initialization Failed", "Failed to initialize UI controller");
            }
        });
    }

    /**
     * @brief Setup controller signal connections
     */
    void setupControllerConnections()
    {
        if (!m_uiController) {
            return;
        }

        auto* mainController = m_uiController->mainController();
        if (!mainController) {
            return;
        }

        // Use modern signal connections with automatic cleanup
        m_connectionManager->connectLambda(
            mainController, &MainController::errorOccurred,
            [this](const QString& title, const QString& message) {
                onControllerError(title, message);
            }
        );

        m_connectionManager->connectLambda(
            mainController, &MainController::statusMessageChanged,
            [this](const QString& message) {
                statusBar()->showMessage(message);
            }
        );

        m_connectionManager->connectLambda(
            mainController, &MainController::musicImportCompleted,
            [this](bool success, int count) {
                onLongOperationCompleted();
                if (success) {
                    statusBar()->showMessage(
                        tr("Successfully imported %1 music files").arg(count), 3000
                    );
                }
            }
        );

        // Connect playback state changes to update UI
        m_connectionManager->connectLambda(
            mainController, &MainController::playbackStateChanged,
            [this](bool playing) {
                // Update play button text and state
                if (ui && ui->btPlay) {
                    ui->btPlay->setText(playing ? tr("Pause") : tr("Play"));
                }
            }
        );
    }

    /**
     * @brief Connect additional UI elements not handled automatically
     */
    void connectAdditionalUIElements()
    {
        if (!ui || !m_uiController) {
            return;
        }

        // Connect menu actions to controller methods
        if (ui->actionAdd_a_single_song) {
            m_connectionManager->connect(
                ui->actionAdd_a_single_song, &QAction::triggered,
                m_uiController.get(), &PlayerUIController::onImportMusicRequested
            );
        }

        if (ui->actionAdd_all_songs_in_a_folder) {
            m_connectionManager->connect(
                ui->actionAdd_all_songs_in_a_folder, &QAction::triggered,
                m_uiController.get(), &PlayerUIController::onImportDirectoryRequested
            );
        }

        if (ui->actionSave_Playlist) {
            m_connectionManager->connect(
                ui->actionSave_Playlist, &QAction::triggered,
                m_uiController.get(), &PlayerUIController::onSavePlaylistRequested
            );
        }

        if (ui->actionLoad_Playlist) {
            m_connectionManager->connect(
                ui->actionLoad_Playlist, &QAction::triggered,
                m_uiController.get(), &PlayerUIController::onLoadPlaylistRequested
            );
        }

        // Connect context menu actions
        setupContextMenuIntegration();
    }

    /**
     * @brief Setup context menu integration with controller
     */
    void setupContextMenuIntegration()
    {
        // This would connect context menu actions to controller methods
        // Implementation depends on the specific context menus in the UI
    }

    /**
     * @brief Setup the progress indicator widget
     */
    void setupProgressIndicator()
    {
        if (!ui || m_progressIndicator) {
            return; // Already setup or no UI
        }

        // Create progress indicator widget
        m_progressIndicator = new ProgressIndicatorWidget(this);
        m_progressIndicator->setCancelEnabled(true);
        m_progressIndicator->setShowElapsedTime(true);
        m_progressIndicator->setShowEstimatedTime(true);

        // Add to the main layout (this depends on the specific UI layout)
        if (ui->centralwidget && ui->centralwidget->layout()) {
            auto* layout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
            if (!layout) {
                // Create a new layout if needed
                layout = new QVBoxLayout(ui->centralwidget);
            }
            
            // Insert at the top of the layout
            layout->insertWidget(0, m_progressIndicator);
        }

        // Connect progress indicator signals
        m_connectionManager->connectLambda(
            m_progressIndicator, &ProgressIndicatorWidget::cancelRequested,
            [this]() {
                // Handle cancellation of long-running operations
                if (m_uiController && m_uiController->mainController()) {
                    // This would cancel the current operation
                    // Implementation depends on the specific operation being cancelled
                }
                m_progressIndicator->hideProgress();
            }
        );

        // Initially hidden
        m_progressIndicator->hide();
    }

    /**
     * @brief Enable UI elements that depend on the controller
     */
    void enableControllerDependentUI()
    {
        if (!ui) {
            return;
        }

        // Enable buttons and menu items that require the controller
        if (ui->btPlay) {
            ui->btPlay->setEnabled(true);
        }
        
        if (ui->btStop) {
            ui->btStop->setEnabled(true);
        }

        if (ui->sliderVolume) {
            ui->sliderVolume->setEnabled(true);
        }

        // Enable menu actions
        if (ui->actionAdd_a_single_song) {
            ui->actionAdd_a_single_song->setEnabled(true);
        }

        if (ui->actionAdd_all_songs_in_a_folder) {
            ui->actionAdd_all_songs_in_a_folder->setEnabled(true);
        }
    }

private:
    std::unique_ptr<PlayerUIController> m_uiController;
    ProgressIndicatorWidget* m_progressIndicator;
    std::unique_ptr<ConnectionManager> m_connectionManager;
};

/**
 * @brief Factory function to create an enhanced player instance
 * @param parent Parent widget
 * @return Pointer to the enhanced player instance
 */
EnhancedPlayer* createEnhancedPlayer(QWidget* parent = nullptr)
{
    return new EnhancedPlayer(parent);
}

/**
 * @brief Example of how to migrate existing player usage
 * 
 * This function demonstrates how existing code that creates
 * player instances can be updated to use the enhanced version.
 */
void migrateExistingPlayerUsage()
{
    // Old way:
    // player* mainWindow = new player();
    
    // New way:
    EnhancedPlayer* mainWindow = createEnhancedPlayer();
    
    // The enhanced player maintains compatibility with the original player interface
    // while adding the new controller pattern functionality
    
    // Access controller functionality when needed
    if (auto* controller = mainWindow->uiController()) {
        // Use controller methods for advanced functionality
        auto* mainController = controller->mainController();
        if (mainController) {
            // Access services through the main controller
            auto* audioService = mainController->audioService();
            auto* dbService = mainController->databaseService();
            // etc.
        }
    }
    
    // Show the window as usual
    mainWindow->show();
}

#include "PlayerIntegration.moc"