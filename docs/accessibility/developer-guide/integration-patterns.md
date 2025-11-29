# XFB Accessibility Integration Patterns

## Overview

This document provides detailed integration patterns for incorporating accessibility features into XFB components. These patterns ensure consistent accessibility implementation across the application.

## Widget Enhancement Patterns

### Pattern 1: Automatic Enhancement

For standard Qt widgets that need basic accessibility support:

```cpp
// The widget will be automatically enhanced when shown
class MyDialog : public QDialog {
    Q_OBJECT
public:
    MyDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setupUI();
        // No explicit accessibility code needed - automatic enhancement handles it
    }
    
private:
    void setupUI() {
        QPushButton* okButton = new QPushButton("OK", this);
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        
        // Set object names for automatic enhancement
        okButton->setObjectName("okButton");
        cancelButton->setObjectName("cancelButton");
        
        // Standard Qt layout
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addWidget(okButton);
        layout->addWidget(cancelButton);
    }
};
```

### Pattern 2: Explicit Enhancement

For widgets requiring specific accessibility metadata:

```cpp
class PlaylistWidget : public QListWidget {
    Q_OBJECT
public:
    PlaylistWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setupAccessibility();
    }
    
private:
    void setupAccessibility() {
        // Register with accessibility manager
        AccessibilityMetadata metadata;
        metadata.name = "Playlist";
        metadata.description = "List of tracks in current playlist";
        metadata.role = QAccessible::List;
        metadata.helpText = "Use arrow keys to navigate, Enter to play selected track";
        
        AccessibilityManager::instance()->registerAccessibleWidget(this, metadata);
        
        // Connect signals for dynamic updates
        connect(this, &QListWidget::itemChanged, this, &PlaylistWidget::onItemChanged);
    }
    
private slots:
    void onItemChanged(QListWidgetItem* item) {
        // Announce playlist changes
        AudioFeedbackService::instance()->announceAction(
            QString("Playlist item updated: %1").arg(item->text())
        );
    }
};
```

### Pattern 3: Custom Accessible Interface

For complex widgets requiring specialized accessibility behavior:

```cpp
class MusicLibraryGrid : public QTableView {
    Q_OBJECT
public:
    MusicLibraryGrid(QWidget* parent = nullptr) : QTableView(parent) {
        installAccessibleInterface();
        setupKeyboardNavigation();
    }
    
private:
    void installAccessibleInterface() {
        // Install custom accessible interface factory
        QAccessible::installFactory([](const QString &classname, QObject *object) -> QAccessibleInterface* {
            if (classname == "MusicLibraryGrid") {
                return new MusicLibraryGridAccessible(qobject_cast<MusicLibraryGrid*>(object));
            }
            return nullptr;
        });
    }
    
    void setupKeyboardNavigation() {
        // Enable keyboard navigation
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        
        // Install event filter for custom navigation
        installEventFilter(new GridNavigationFilter(this));
    }
};

class MusicLibraryGridAccessible : public QAccessibleTable {
public:
    MusicLibraryGridAccessible(MusicLibraryGrid* grid) : QAccessibleTable(grid) {}
    
    QString text(QAccessible::Text t) const override {
        switch (t) {
        case QAccessible::Name:
            return "Music Library";
        case QAccessible::Description:
            return QString("Music library with %1 tracks").arg(view()->model()->rowCount());
        default:
            return QAccessibleTable::text(t);
        }
    }
    
    QAccessibleInterface* cellAt(int row, int column) const override {
        QModelIndex index = view()->model()->index(row, column);
        if (!index.isValid()) return nullptr;
        
        return new MusicLibraryCellAccessible(view(), index);
    }
    
private:
    QTableView* view() const { return qobject_cast<QTableView*>(object()); }
};
```

## Audio Feedback Integration Patterns

### Pattern 1: Action Confirmation

For immediate feedback on user actions:

```cpp
class PlaybackControls : public QWidget {
    Q_OBJECT
public:
    PlaybackControls(QWidget* parent = nullptr) : QWidget(parent) {
        setupControls();
        connectAudioFeedback();
    }
    
private:
    void setupControls() {
        playButton = new QPushButton("Play", this);
        pauseButton = new QPushButton("Pause", this);
        stopButton = new QPushButton("Stop", this);
    }
    
    void connectAudioFeedback() {
        AudioFeedbackService* feedback = AudioFeedbackService::instance();
        
        connect(playButton, &QPushButton::clicked, [feedback]() {
            feedback->announceAction("Playback started");
        });
        
        connect(pauseButton, &QPushButton::clicked, [feedback]() {
            feedback->announceAction("Playback paused");
        });
        
        connect(stopButton, &QPushButton::clicked, [feedback]() {
            feedback->announceAction("Playback stopped");
        });
    }
    
private:
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
};
```

### Pattern 2: Progress Feedback

For long-running operations:

```cpp
class DatabaseImporter : public QObject {
    Q_OBJECT
public:
    void importMusicLibrary(const QString& path) {
        BackgroundOperationFeedback* feedback = BackgroundOperationFeedback::instance();
        
        // Start progress tracking
        feedback->startOperation("Importing music library", 100);
        
        // Simulate import process
        QTimer* timer = new QTimer(this);
        int progress = 0;
        
        connect(timer, &QTimer::timeout, [this, feedback, &progress, timer]() {
            progress += 10;
            feedback->updateProgress(progress, QString("Imported %1 files").arg(progress));
            
            if (progress >= 100) {
                timer->stop();
                feedback->completeOperation("Music library import completed successfully");
                timer->deleteLater();
            }
        });
        
        timer->start(500); // Update every 500ms
    }
};
```

### Pattern 3: Error Handling

For error announcements:

```cpp
class FileOperations : public QObject {
    Q_OBJECT
public:
    bool savePlaylist(const QString& filename) {
        try {
            // Attempt to save playlist
            performSave(filename);
            
            AudioFeedbackService::instance()->announceSuccess(
                QString("Playlist saved as %1").arg(QFileInfo(filename).baseName())
            );
            return true;
            
        } catch (const std::exception& e) {
            AudioFeedbackService::instance()->announceError(
                QString("Failed to save playlist: %1").arg(e.what())
            );
            return false;
        }
    }
    
private:
    void performSave(const QString& filename) {
        // Implementation details...
    }
};
```

## Keyboard Navigation Integration Patterns

### Pattern 1: Standard Navigation Enhancement

For widgets requiring enhanced keyboard support:

```cpp
class EnhancedListWidget : public QListWidget {
    Q_OBJECT
public:
    EnhancedListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setupKeyboardNavigation();
    }
    
protected:
    void keyPressEvent(QKeyEvent* event) override {
        KeyboardNavigationController* nav = KeyboardNavigationController::instance();
        
        switch (event->key()) {
        case Qt::Key_Home:
            setCurrentRow(0);
            nav->announceNavigation("First item");
            break;
            
        case Qt::Key_End:
            setCurrentRow(count() - 1);
            nav->announceNavigation("Last item");
            break;
            
        case Qt::Key_Delete:
            if (currentItem()) {
                QString itemText = currentItem()->text();
                delete takeItem(currentRow());
                nav->announceAction(QString("Deleted %1").arg(itemText));
            }
            break;
            
        default:
            QListWidget::keyPressEvent(event);
        }
    }
    
private:
    void setupKeyboardNavigation() {
        setFocusPolicy(Qt::StrongFocus);
        
        // Register keyboard shortcuts
        KeyboardNavigationController* nav = KeyboardNavigationController::instance();
        nav->registerShortcut(this, QKeySequence::Delete, "Delete selected item");
        nav->registerShortcut(this, QKeySequence::MoveToStartOfDocument, "Go to first item");
        nav->registerShortcut(this, QKeySequence::MoveToEndOfDocument, "Go to last item");
    }
};
```

### Pattern 2: Grid Navigation

For table and grid widgets:

```cpp
class AccessibleTableWidget : public QTableWidget {
    Q_OBJECT
public:
    AccessibleTableWidget(QWidget* parent = nullptr) : QTableWidget(parent) {
        setupGridNavigation();
    }
    
protected:
    void keyPressEvent(QKeyEvent* event) override {
        KeyboardNavigationController* nav = KeyboardNavigationController::instance();
        
        switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
            QTableWidget::keyPressEvent(event);
            announceCurrentCell();
            break;
            
        case Qt::Key_F2:
            if (currentItem()) {
                editItem(currentItem());
                nav->announceAction("Editing cell");
            }
            break;
            
        case Qt::Key_Escape:
            if (state() == QAbstractItemView::EditingState) {
                nav->announceAction("Edit cancelled");
            }
            QTableWidget::keyPressEvent(event);
            break;
            
        default:
            QTableWidget::keyPressEvent(event);
        }
    }
    
private:
    void setupGridNavigation() {
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setTabKeyNavigation(false); // Use arrow keys for navigation
        
        // Connect to selection changes
        connect(this, &QTableWidget::currentCellChanged, 
                this, &AccessibleTableWidget::onCellChanged);
    }
    
    void announceCurrentCell() {
        int row = currentRow();
        int col = currentColumn();
        
        if (row >= 0 && col >= 0) {
            QString header = horizontalHeaderItem(col) ? 
                           horizontalHeaderItem(col)->text() : 
                           QString("Column %1").arg(col + 1);
            
            QString content = item(row, col) ? 
                            item(row, col)->text() : 
                            "Empty";
            
            AudioFeedbackService::instance()->announceNavigation(
                QString("%1: %2, Row %3").arg(header, content).arg(row + 1)
            );
        }
    }
    
private slots:
    void onCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) {
        Q_UNUSED(previousRow)
        Q_UNUSED(previousColumn)
        
        if (currentRow >= 0 && currentColumn >= 0) {
            announceCurrentCell();
        }
    }
};
```

## Live Region Integration Patterns

### Pattern 1: Status Updates

For dynamic status information:

```cpp
class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    StatusBar(QWidget* parent = nullptr) : QStatusBar(parent) {
        setupLiveRegions();
    }
    
    void updatePlaybackStatus(const QString& status) {
        playbackLabel->setText(status);
        liveRegionManager->updateLiveRegion(playbackLabel, status);
    }
    
    void updateRecordingStatus(const QString& status) {
        recordingLabel->setText(status);
        liveRegionManager->updateLiveRegion(recordingLabel, status);
    }
    
private:
    void setupLiveRegions() {
        playbackLabel = new QLabel("Ready", this);
        recordingLabel = new QLabel("Not recording", this);
        
        addWidget(playbackLabel);
        addWidget(recordingLabel);
        
        liveRegionManager = LiveRegionManager::instance();
        
        // Register as polite live regions (won't interrupt other announcements)
        liveRegionManager->createLiveRegion(playbackLabel, QAccessible::LiveRegion::Polite);
        liveRegionManager->createLiveRegion(recordingLabel, QAccessible::LiveRegion::Polite);
    }
    
private:
    QLabel* playbackLabel;
    QLabel* recordingLabel;
    LiveRegionManager* liveRegionManager;
};
```

### Pattern 2: Critical Alerts

For urgent notifications:

```cpp
class AlertManager : public QObject {
    Q_OBJECT
public:
    void showCriticalAlert(const QString& message) {
        // Create alert dialog
        QMessageBox* alert = new QMessageBox(QMessageBox::Critical, "Critical Alert", message);
        
        // Register as assertive live region (will interrupt other announcements)
        LiveRegionManager::instance()->createLiveRegion(alert, QAccessible::LiveRegion::Assertive);
        
        // Immediate audio announcement
        AudioFeedbackService::instance()->announceCritical(message);
        
        alert->exec();
        alert->deleteLater();
    }
    
    void showProgressAlert(const QString& operation, int percentage) {
        QString message = QString("%1: %2% complete").arg(operation).arg(percentage);
        
        // Update existing progress region or create new one
        if (!progressLabel) {
            progressLabel = new QLabel();
            LiveRegionManager::instance()->createLiveRegion(progressLabel, QAccessible::LiveRegion::Polite);
        }
        
        progressLabel->setText(message);
        LiveRegionManager::instance()->updateLiveRegion(progressLabel, message);
    }
    
private:
    QLabel* progressLabel = nullptr;
};
```

## Settings Integration Patterns

### Pattern 1: Accessibility Preferences

For components that need to respond to accessibility settings:

```cpp
class AccessibilityAwareWidget : public QWidget {
    Q_OBJECT
public:
    AccessibilityAwareWidget(QWidget* parent = nullptr) : QWidget(parent) {
        connectToSettings();
        applyCurrentSettings();
    }
    
private:
    void connectToSettings() {
        AccessibilitySettingsService* settings = AccessibilitySettingsService::instance();
        
        connect(settings, &AccessibilitySettingsService::verbosityLevelChanged,
                this, &AccessibilityAwareWidget::onVerbosityChanged);
        
        connect(settings, &AccessibilitySettingsService::keyboardShortcutsChanged,
                this, &AccessibilityAwareWidget::onShortcutsChanged);
    }
    
    void applyCurrentSettings() {
        AccessibilitySettingsService* settings = AccessibilitySettingsService::instance();
        
        // Apply verbosity level
        onVerbosityChanged(settings->verbosityLevel());
        
        // Apply keyboard shortcuts
        onShortcutsChanged(settings->keyboardShortcuts());
    }
    
private slots:
    void onVerbosityChanged(AccessibilitySettingsService::VerbosityLevel level) {
        switch (level) {
        case AccessibilitySettingsService::Terse:
            // Minimal announcements
            setAccessibleDescription("");
            break;
        case AccessibilitySettingsService::Normal:
            // Standard announcements
            setAccessibleDescription("Standard widget description");
            break;
        case AccessibilitySettingsService::Verbose:
            // Detailed announcements
            setAccessibleDescription("Detailed widget description with usage hints");
            break;
        }
    }
    
    void onShortcutsChanged(const QMap<QString, QKeySequence>& shortcuts) {
        // Update widget-specific shortcuts
        if (shortcuts.contains("myAction")) {
            myActionShortcut->setKey(shortcuts["myAction"]);
        }
    }
    
private:
    QShortcut* myActionShortcut;
};
```

## Error Handling Patterns

### Pattern 1: Graceful Degradation

For handling accessibility service failures:

```cpp
class RobustAccessibilityWidget : public QWidget {
    Q_OBJECT
public:
    RobustAccessibilityWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setupAccessibilityWithFallback();
    }
    
private:
    void setupAccessibilityWithFallback() {
        try {
            // Attempt full accessibility setup
            AccessibilityManager::instance()->registerAccessibleWidget(this, getMetadata());
            AudioFeedbackService::instance()->connectWidget(this);
            KeyboardNavigationController::instance()->enhanceWidget(this);
            
            accessibilityEnabled = true;
            
        } catch (const std::exception& e) {
            // Fall back to basic Qt accessibility
            Logger::instance()->logWarning("Accessibility", 
                QString("Failed to setup enhanced accessibility: %1").arg(e.what()));
            
            setupBasicAccessibility();
            accessibilityEnabled = false;
        }
    }
    
    void setupBasicAccessibility() {
        // Minimal accessibility using only Qt built-ins
        setAccessibleName("Widget Name");
        setAccessibleDescription("Basic description");
        setFocusPolicy(Qt::StrongFocus);
    }
    
    AccessibilityMetadata getMetadata() const {
        AccessibilityMetadata metadata;
        metadata.name = "Enhanced Widget";
        metadata.description = "Widget with full accessibility support";
        metadata.role = QAccessible::Button;
        return metadata;
    }
    
private:
    bool accessibilityEnabled = false;
};
```

## Performance Optimization Patterns

### Pattern 1: Lazy Initialization

For expensive accessibility operations:

```cpp
class PerformantAccessibilityWidget : public QWidget {
    Q_OBJECT
public:
    PerformantAccessibilityWidget(QWidget* parent = nullptr) : QWidget(parent) {
        // Defer accessibility setup until needed
    }
    
protected:
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        
        if (!accessibilityInitialized) {
            initializeAccessibility();
            accessibilityInitialized = true;
        }
    }
    
    void focusInEvent(QFocusEvent* event) override {
        QWidget::focusInEvent(event);
        
        // Ensure accessibility is ready when widget receives focus
        if (!accessibilityInitialized) {
            initializeAccessibility();
            accessibilityInitialized = true;
        }
    }
    
private:
    void initializeAccessibility() {
        // Perform expensive accessibility setup only when needed
        AccessibilityManager::instance()->registerAccessibleWidget(this, getMetadata());
        
        // Cache frequently accessed accessibility information
        cachedAccessibleName = calculateAccessibleName();
        cachedAccessibleDescription = calculateAccessibleDescription();
    }
    
    QString calculateAccessibleName() const {
        // Expensive calculation...
        return "Calculated Name";
    }
    
    QString calculateAccessibleDescription() const {
        // Expensive calculation...
        return "Calculated Description";
    }
    
    AccessibilityMetadata getMetadata() const {
        AccessibilityMetadata metadata;
        metadata.name = cachedAccessibleName;
        metadata.description = cachedAccessibleDescription;
        return metadata;
    }
    
private:
    bool accessibilityInitialized = false;
    QString cachedAccessibleName;
    QString cachedAccessibleDescription;
};
```

These patterns provide a comprehensive foundation for integrating accessibility features throughout the XFB application while maintaining performance and reliability.