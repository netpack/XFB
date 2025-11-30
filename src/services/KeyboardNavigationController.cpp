#include "KeyboardNavigationController.h"
#include "AccessibilityManager.h"
#include "Logger.h"
#include <QApplication>
#include <QKeyEvent>
#include <QTableView>
#include <QListWidget>
#include <QTreeView>
#include <QHeaderView>
#include <QAbstractItemModel>
#include <QScrollBar>
#include <QDebug>

KeyboardNavigationController::KeyboardNavigationController(AccessibilityManager* accessibilityManager, QObject* parent)
    : QObject(parent)
    , m_accessibilityManager(accessibilityManager)
    , m_lastFocusedWidget(nullptr)
    , m_focusAnnouncementTimer(new QTimer(this))
    , m_initialized(false)
    , m_eventFilterInstalled(false)
{
    if (!m_accessibilityManager) {
        logError("KeyboardNavigationController: AccessibilityManager is null");
        return;
    }
    
    // Setup focus announcement timer
    m_focusAnnouncementTimer->setSingleShot(true);
    m_focusAnnouncementTimer->setInterval(FOCUS_ANNOUNCEMENT_DELAY_MS);
    connect(m_focusAnnouncementTimer, &QTimer::timeout,
            this, &KeyboardNavigationController::processFocusAnnouncement);
    
    logDebug("KeyboardNavigationController created");
}

KeyboardNavigationController::~KeyboardNavigationController()
{
    shutdown();
    logDebug("KeyboardNavigationController destroyed");
}

bool KeyboardNavigationController::initialize()
{
    if (m_initialized) {
        logWarning("KeyboardNavigationController already initialized");
        return true;
    }
    
    logDebug("Initializing KeyboardNavigationController");
    
    // Connect to application focus changes
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        connect(app, &QApplication::focusChanged,
                this, &KeyboardNavigationController::onFocusChanged);
    } else {
        logError("Failed to get QApplication instance");
        return false;
    }
    
    // Install application-wide event filter
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->installEventFilter(this);
        m_eventFilterInstalled = true;
    }
    
    // Setup default keyboard shortcuts
    setupDefaultShortcuts();
    
    m_initialized = true;
    logDebug("KeyboardNavigationController initialized successfully");
    return true;
}

void KeyboardNavigationController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    logDebug("Shutting down KeyboardNavigationController");
    
    // Remove event filter
    if (m_eventFilterInstalled) {
        if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
            app->removeEventFilter(this);
        }
        m_eventFilterInstalled = false;
    }
    
    // Stop timers
    m_focusAnnouncementTimer->stop();
    
    // Clear data structures
    m_shortcuts.clear();
    m_sequenceToAction.clear();
    m_navigationWidgets.clear();
    m_arrowKeyEnabled.clear();
    m_customTabOrder.clear();
    
    m_lastFocusedWidget = nullptr;
    m_initialized = false;
    
    logDebug("KeyboardNavigationController shutdown complete");
}

bool KeyboardNavigationController::registerKeyboardShortcut(const QString& action,
                                                           const QKeySequence& sequence,
                                                           const QString& description,
                                                           const QString& context,
                                                           bool customizable,
                                                           QWidget* targetWidget)
{
    if (action.isEmpty() || sequence.isEmpty()) {
        logWarning("Cannot register keyboard shortcut with empty action or sequence");
        return false;
    }
    
    // Check for conflicts
    if (hasShortcutConflict(sequence, action)) {
        logWarning(QString("Keyboard shortcut conflict detected for sequence: %1")
                   .arg(sequence.toString()));
        return false;
    }
    
    // Remove existing shortcut with same action
    if (m_shortcuts.contains(action)) {
        unregisterKeyboardShortcut(action);
    }
    
    // Create shortcut info
    KeyboardShortcut shortcut;
    shortcut.action = action;
    shortcut.sequence = sequence;
    shortcut.description = description;
    shortcut.context = context.isEmpty() ? SHORTCUT_CONTEXT_GLOBAL : context;
    shortcut.customizable = customizable;
    shortcut.targetWidget = targetWidget;
    
    // Register shortcut
    m_shortcuts[action] = shortcut;
    m_sequenceToAction[sequence] = action;
    
    logDebug(QString("Registered keyboard shortcut: %1 (%2) - %3")
             .arg(action)
             .arg(sequence.toString())
             .arg(description));
    
    return true;
}

void KeyboardNavigationController::unregisterKeyboardShortcut(const QString& action)
{
    if (!m_shortcuts.contains(action)) {
        return;
    }
    
    KeyboardShortcut shortcut = m_shortcuts[action];
    m_sequenceToAction.remove(shortcut.sequence);
    m_shortcuts.remove(action);
    
    logDebug(QString("Unregistered keyboard shortcut: %1").arg(action));
}

KeyboardNavigationController::KeyboardShortcut KeyboardNavigationController::getKeyboardShortcut(const QString& action) const
{
    return m_shortcuts.value(action, KeyboardShortcut());
}

void KeyboardNavigationController::setCustomTabOrder(QWidget* first, QWidget* second)
{
    if (!first || !second) {
        logWarning("Cannot set tab order with null widgets");
        return;
    }
    
    m_customTabOrder[first] = second;
    QWidget::setTabOrder(first, second);
    
    logDebug(QString("Set custom tab order: %1 -> %2")
             .arg(first->objectName())
             .arg(second->objectName()));
}

void KeyboardNavigationController::registerNavigationWidget(QWidget* widget,
                                                           NavigationContext context,
                                                           const QString& accessibleName,
                                                           const QString& navigationHint)
{
    if (!widget) {
        logWarning("Cannot register null widget for navigation");
        return;
    }
    
    FocusNavigationInfo info;
    info.widget = widget;
    info.context = context;
    info.accessibleName = accessibleName;
    info.navigationHint = navigationHint;
    
    m_navigationWidgets[widget] = info;
    
    // Connect to widget destruction
    connect(widget, &QObject::destroyed,
            this, &KeyboardNavigationController::onWidgetDestroyed);
    
    // Install event filter on widget
    installEventFilterRecursively(widget);
    
    logDebug(QString("Registered navigation widget: %1 (context: %2)")
             .arg(accessibleName)
             .arg(static_cast<int>(context)));
}

void KeyboardNavigationController::unregisterNavigationWidget(QWidget* widget)
{
    if (!widget || !m_navigationWidgets.contains(widget)) {
        return;
    }
    
    // Remove event filter
    removeEventFilterRecursively(widget);
    
    // Remove from data structures
    m_navigationWidgets.remove(widget);
    m_arrowKeyEnabled.remove(widget);
    m_customTabOrder.remove(widget);
    
    // Remove widget from custom tab order values
    auto it = m_customTabOrder.begin();
    while (it != m_customTabOrder.end()) {
        if (it.value() == widget) {
            it = m_customTabOrder.erase(it);
        } else {
            ++it;
        }
    }
    
    disconnect(widget, &QObject::destroyed,
              this, &KeyboardNavigationController::onWidgetDestroyed);
    
    logDebug("Unregistered navigation widget");
}

void KeyboardNavigationController::setArrowKeyNavigationEnabled(QWidget* widget, bool enabled)
{
    if (!widget) {
        return;
    }
    
    m_arrowKeyEnabled[widget] = enabled;
    
    logDebug(QString("Arrow key navigation %1 for widget: %2")
             .arg(enabled ? "enabled" : "disabled")
             .arg(widget->objectName()));
}

bool KeyboardNavigationController::isArrowKeyNavigationEnabled(QWidget* widget) const
{
    return m_arrowKeyEnabled.value(widget, false);
}

KeyboardNavigationController::NavigationContext KeyboardNavigationController::getNavigationContext(QWidget* widget) const
{
    if (!widget || !m_navigationWidgets.contains(widget)) {
        return NavigationContext::General;
    }
    
    return m_navigationWidgets[widget].context;
}

void KeyboardNavigationController::announceFocusInfo(QWidget* widget)
{
    if (!widget || !m_accessibilityManager || !m_accessibilityManager->isAccessibilityEnabled()) {
        return;
    }
    
    FocusNavigationInfo info = m_navigationWidgets.value(widget, FocusNavigationInfo());
    
    QString announcement = info.accessibleName;
    if (announcement.isEmpty()) {
        announcement = widget->accessibleName();
    }
    if (announcement.isEmpty()) {
        announcement = widget->objectName();
    }
    
    // Add navigation hint for verbose mode
    if (m_accessibilityManager->verbosityLevel() == AccessibilityManager::VerbosityLevel::Verbose &&
        !info.navigationHint.isEmpty()) {
        announcement += ", " + info.navigationHint;
    }
    
    if (!announcement.isEmpty()) {
        m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
    }
}

void KeyboardNavigationController::onFocusChanged(QWidget* old, QWidget* now)
{
    Q_UNUSED(old)
    
    if (!now) {
        return;
    }
    
    m_lastFocusedWidget = now;
    
    // Check if navigation context changed
    NavigationContext newContext = getNavigationContext(now);
    emit navigationContextChanged(now, newContext);
    
    // Schedule focus announcement
    if (m_navigationWidgets.contains(now)) {
        FocusNavigationInfo info = m_navigationWidgets[now];
        m_pendingFocusAnnouncement = info.accessibleName;
        
        if (m_pendingFocusAnnouncement.isEmpty()) {
            m_pendingFocusAnnouncement = now->accessibleName();
        }
        
        m_focusAnnouncementTimer->start();
    }
}

void KeyboardNavigationController::executeShortcutAction(const QString& action)
{
    if (!m_shortcuts.contains(action)) {
        logWarning(QString("Unknown shortcut action: %1").arg(action));
        return;
    }
    
    KeyboardShortcut shortcut = m_shortcuts[action];
    
    logDebug(QString("Executing shortcut action: %1").arg(action));
    
    // Emit signal for action execution
    emit shortcutTriggered(action, shortcut.sequence);
    
    // If target widget is specified, send key event to it
    if (shortcut.targetWidget) {
        QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier);
        QApplication::sendEvent(shortcut.targetWidget, &keyEvent);
    }
}

bool KeyboardNavigationController::eventFilter(QObject* obj, QEvent* event)
{
    if (!m_initialized || event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(obj, event);
    }
    
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget) {
        return QObject::eventFilter(obj, event);
    }
    
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    
    // Handle keyboard shortcuts first
    QKeySequence sequence(keyEvent->key() | keyEvent->modifiers());
    if (m_sequenceToAction.contains(sequence)) {
        QString action = m_sequenceToAction[sequence];
        executeShortcutAction(action);
        return true; // Event handled
    }
    
    // Handle navigation keys
    if (handleKeyPress(widget, keyEvent)) {
        return true; // Event handled
    }
    
    return QObject::eventFilter(obj, event);
}

void KeyboardNavigationController::onWidgetDestroyed(QObject* obj)
{
    QWidget* widget = static_cast<QWidget*>(obj);
    unregisterNavigationWidget(widget);
}

void KeyboardNavigationController::processFocusAnnouncement()
{
    if (!m_pendingFocusAnnouncement.isEmpty() && m_accessibilityManager) {
        m_accessibilityManager->announceMessage(m_pendingFocusAnnouncement, 
                                               AccessibilityManager::Priority::Normal);
        m_pendingFocusAnnouncement.clear();
    }
}

bool KeyboardNavigationController::handleKeyPress(QWidget* widget, QKeyEvent* event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());
    
    // Handle arrow keys
    if (key >= Qt::Key_Left && key <= Qt::Key_Down) {
        if (isArrowKeyNavigationEnabled(widget) || m_navigationWidgets.contains(widget)) {
            return handleArrowKeyNavigation(widget, key);
        }
    }
    
    // Handle tab navigation
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        bool forward = (key == Qt::Key_Tab);
        return handleTabNavigation(widget, forward);
    }
    
    // Handle Enter/Return for activation
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Activated", AccessibilityManager::Priority::Normal);
        }
    }
    
    // Handle Escape for cancellation
    if (key == Qt::Key_Escape) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Cancelled", AccessibilityManager::Priority::Normal);
        }
    }
    
    return false; // Let default handling proceed
}

bool KeyboardNavigationController::handleArrowKeyNavigation(QWidget* widget, Qt::Key key)
{
    // Handle grid navigation (tables, tree views)
    if (QAbstractItemView* view = qobject_cast<QAbstractItemView*>(widget)) {
        return handleGridNavigation(view, key);
    }
    
    // Handle list navigation
    if (QListWidget* list = qobject_cast<QListWidget*>(widget)) {
        return handleListNavigation(list, key);
    }
    
    return false; // Let default handling proceed
}

bool KeyboardNavigationController::handleGridNavigation(QAbstractItemView* view, Qt::Key key)
{
    if (!view || !view->model()) {
        return false;
    }
    
    QModelIndex currentIndex = view->currentIndex();
    QModelIndex newIndex;
    
    switch (key) {
        case Qt::Key_Up:
            newIndex = view->model()->index(currentIndex.row() - 1, currentIndex.column(), currentIndex.parent());
            break;
        case Qt::Key_Down:
            newIndex = view->model()->index(currentIndex.row() + 1, currentIndex.column(), currentIndex.parent());
            break;
        case Qt::Key_Left:
            newIndex = view->model()->index(currentIndex.row(), currentIndex.column() - 1, currentIndex.parent());
            break;
        case Qt::Key_Right:
            newIndex = view->model()->index(currentIndex.row(), currentIndex.column() + 1, currentIndex.parent());
            break;
        default:
            return false;
    }
    
    if (newIndex.isValid()) {
        view->setCurrentIndex(newIndex);
        announceGridCell(view, newIndex);
        
        // Emit navigation change signal
        QString itemText = newIndex.data(Qt::DisplayRole).toString();
        emit gridNavigationChanged(view, newIndex.row(), newIndex.column(), itemText);
        
        return true; // Event handled
    }
    
    return false; // Let default handling proceed
}

bool KeyboardNavigationController::handleListNavigation(QListWidget* list, Qt::Key key)
{
    if (!list) {
        return false;
    }
    
    int currentRow = list->currentRow();
    int newRow = currentRow;
    
    switch (key) {
        case Qt::Key_Up:
            newRow = currentRow - 1;
            break;
        case Qt::Key_Down:
            newRow = currentRow + 1;
            break;
        default:
            return false;
    }
    
    if (newRow >= 0 && newRow < list->count()) {
        list->setCurrentRow(newRow);
        announceListItem(list, newRow);
        
        // Emit navigation change signal
        QString itemText = list->item(newRow) ? list->item(newRow)->text() : QString();
        emit gridNavigationChanged(list, newRow, 0, itemText);
        
        return true; // Event handled
    }
    
    return false; // Let default handling proceed
}

bool KeyboardNavigationController::handleTabNavigation(QWidget* widget, bool forward)
{
    // Check for custom tab order
    if (forward && m_customTabOrder.contains(widget)) {
        QWidget* nextWidget = m_customTabOrder[widget];
        if (nextWidget && nextWidget->isVisible() && nextWidget->isEnabled()) {
            nextWidget->setFocus();
            return true; // Event handled
        }
    }
    
    // Special handling for table views and list widgets to prevent keyboard traps
    if (QTableView* tableView = qobject_cast<QTableView*>(widget)) {
        // Allow tab navigation out of table views for accessibility
        QWidget* nextWidget = nullptr;
        
        if (forward) {
            // Find next focusable widget after the table
            nextWidget = tableView->nextInFocusChain();
        } else {
            // Find previous focusable widget before the table
            nextWidget = tableView->previousInFocusChain();
        }
        
        // Ensure we don't get stuck in the same table
        while (nextWidget && nextWidget == tableView) {
            if (forward) {
                nextWidget = nextWidget->nextInFocusChain();
            } else {
                nextWidget = nextWidget->previousInFocusChain();
            }
        }
        
        if (nextWidget && nextWidget->isVisible() && nextWidget->isEnabled() && 
            nextWidget->focusPolicy() != Qt::NoFocus) {
            nextWidget->setFocus();
            
            // Announce the focus change for screen readers
            if (m_accessibilityManager) {
                QString announcement = QString("Moved to %1").arg(nextWidget->objectName().isEmpty() ? 
                                                                nextWidget->metaObject()->className() : 
                                                                nextWidget->objectName());
                m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
            }
            
            return true; // Event handled
        }
    }
    
    // Special handling for list widgets to prevent keyboard traps
    if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
        // Allow tab navigation out of list widgets for accessibility
        QWidget* nextWidget = nullptr;
        
        if (forward) {
            // Find next focusable widget after the list
            nextWidget = listWidget->nextInFocusChain();
        } else {
            // Find previous focusable widget before the list
            nextWidget = listWidget->previousInFocusChain();
        }
        
        // Ensure we don't get stuck in the same list
        while (nextWidget && nextWidget == listWidget) {
            if (forward) {
                nextWidget = nextWidget->nextInFocusChain();
            } else {
                nextWidget = nextWidget->previousInFocusChain();
            }
        }
        
        if (nextWidget && nextWidget->isVisible() && nextWidget->isEnabled() && 
            nextWidget->focusPolicy() != Qt::NoFocus) {
            nextWidget->setFocus();
            
            // Announce the focus change for screen readers
            if (m_accessibilityManager) {
                QString announcement = QString("Moved to %1").arg(nextWidget->objectName().isEmpty() ? 
                                                                nextWidget->metaObject()->className() : 
                                                                nextWidget->objectName());
                m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
            }
            
            return true; // Event handled
        }
    }
    
    return false; // Let default handling proceed
}

bool KeyboardNavigationController::hasShortcutConflict(const QKeySequence& sequence, const QString& excludeAction) const
{
    if (m_sequenceToAction.contains(sequence)) {
        QString existingAction = m_sequenceToAction[sequence];
        return existingAction != excludeAction;
    }
    return false;
}

void KeyboardNavigationController::installEventFilterRecursively(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    widget->installEventFilter(this);
    
    // Install on child widgets
    for (QWidget* child : widget->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }
}

void KeyboardNavigationController::removeEventFilterRecursively(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    widget->removeEventFilter(this);
    
    // Remove from child widgets
    for (QWidget* child : widget->findChildren<QWidget*>()) {
        child->removeEventFilter(this);
    }
}

void KeyboardNavigationController::setupDefaultShortcuts()
{
    // Global shortcuts
    registerKeyboardShortcut("help", QKeySequence(Qt::Key_F1), "Show help", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("quit", QKeySequence(Qt::CTRL | Qt::Key_Q), "Quit application", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("preferences", QKeySequence(Qt::CTRL | Qt::Key_Comma), "Open preferences", SHORTCUT_CONTEXT_GLOBAL);
    
    // Navigation shortcuts (Alt+Letter for area navigation)
    registerKeyboardShortcut("menu_bar", QKeySequence(Qt::ALT | Qt::Key_M), "Access main menu bar", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("library", QKeySequence(Qt::ALT | Qt::Key_L), "Jump to music library", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("playlist", QKeySequence(Qt::ALT | Qt::Key_P), "Jump to playlist area", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("controls", QKeySequence(Qt::ALT | Qt::Key_C), "Jump to player controls", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("status", QKeySequence(Qt::ALT | Qt::Key_S), "Jump to status bar", SHORTCUT_CONTEXT_GLOBAL);
    
    // Player shortcuts - both documented and legacy shortcuts
    registerKeyboardShortcut("play_pause", QKeySequence(Qt::Key_Space), "Play/Pause", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("stop", QKeySequence(Qt::CTRL | Qt::Key_Space), "Stop playback", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("next_track", QKeySequence(Qt::CTRL | Qt::Key_Right), "Next track", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("previous_track", QKeySequence(Qt::CTRL | Qt::Key_Left), "Previous track", SHORTCUT_CONTEXT_PLAYER);
    
    // Volume control - both documented and legacy shortcuts
    registerKeyboardShortcut("volume_up", QKeySequence(Qt::CTRL | Qt::Key_Up), "Volume up", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("volume_down", QKeySequence(Qt::CTRL | Qt::Key_Down), "Volume down", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("volume_up_fine", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up), "Volume up (fine)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("volume_down_fine", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down), "Volume down (fine)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("mute", QKeySequence(Qt::CTRL | Qt::Key_M), "Mute/Unmute", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("reset_volume", QKeySequence(Qt::CTRL | Qt::Key_0), "Reset volume to 100%", SHORTCUT_CONTEXT_PLAYER);
    
    // Seeking controls
    registerKeyboardShortcut("fast_forward", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Right), "Seek forward 10 seconds", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("rewind", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Left), "Seek backward 10 seconds", SHORTCUT_CONTEXT_PLAYER);
    
    // Time information shortcuts
    registerKeyboardShortcut("current_time", QKeySequence(Qt::CTRL | Qt::Key_T), "Announce current time", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("remaining_time", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), "Announce remaining time", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("total_duration", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), "Announce total duration", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("track_info", QKeySequence(Qt::CTRL | Qt::Key_D), "Announce track details", SHORTCUT_CONTEXT_PLAYER);
    
    // Legacy shortcuts for compatibility
    registerKeyboardShortcut("next_track_legacy", QKeySequence(Qt::Key_N), "Next track (legacy)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("previous_track_legacy", QKeySequence(Qt::Key_P), "Previous track (legacy)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("volume_up_legacy", QKeySequence(Qt::Key_Plus), "Volume up (legacy)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("volume_down_legacy", QKeySequence(Qt::Key_Minus), "Volume down (legacy)", SHORTCUT_CONTEXT_PLAYER);
    registerKeyboardShortcut("mute_legacy", QKeySequence(Qt::Key_M), "Mute/Unmute (legacy)", SHORTCUT_CONTEXT_PLAYER);
    
    // Database shortcuts
    registerKeyboardShortcut("refresh_database", QKeySequence(Qt::Key_F5), "Refresh database", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("search", QKeySequence(Qt::CTRL | Qt::Key_F), "Search", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("find_next", QKeySequence(Qt::Key_F3), "Find next", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("find_previous", QKeySequence(Qt::SHIFT | Qt::Key_F3), "Find previous", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("advanced_search", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), "Advanced search", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("clear_search", QKeySequence(Qt::Key_Escape), "Clear search", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("add_to_playlist", QKeySequence(Qt::Key_Return), "Add to playlist", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("delete_item", QKeySequence(Qt::Key_Delete), "Delete item", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("track_information", QKeySequence(Qt::CTRL | Qt::Key_I), "Track information", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("rename", QKeySequence(Qt::Key_F2), "Rename", SHORTCUT_CONTEXT_DATABASE);
    
    // Library management
    registerKeyboardShortcut("library_settings", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L), "Library settings", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("add_directory", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A), "Add directory", SHORTCUT_CONTEXT_DATABASE);
    registerKeyboardShortcut("refresh_library", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), "Refresh library", SHORTCUT_CONTEXT_DATABASE);
    
    // Playlist shortcuts
    registerKeyboardShortcut("new_playlist", QKeySequence(Qt::CTRL | Qt::Key_N), "New playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("new_smart_playlist", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), "New smart playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("save_playlist", QKeySequence(Qt::CTRL | Qt::Key_S), "Save playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("save_as_playlist", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), "Save playlist as", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("load_playlist", QKeySequence(Qt::CTRL | Qt::Key_O), "Load playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("close_playlist", QKeySequence(Qt::CTRL | Qt::Key_W), "Close playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("duplicate_playlist", QKeySequence(Qt::CTRL | Qt::Key_D), "Duplicate playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("rename_playlist", QKeySequence(Qt::CTRL | Qt::Key_R), "Rename playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("clear_playlist", QKeySequence(Qt::CTRL | Qt::Key_L), "Clear playlist", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("cut", QKeySequence(Qt::CTRL | Qt::Key_X), "Cut", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("copy", QKeySequence(Qt::CTRL | Qt::Key_C), "Copy", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("paste", QKeySequence(Qt::CTRL | Qt::Key_V), "Paste", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("select_all", QKeySequence(Qt::CTRL | Qt::Key_A), "Select all", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("undo", QKeySequence(Qt::CTRL | Qt::Key_Z), "Undo", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("redo", QKeySequence(Qt::CTRL | Qt::Key_Y), "Redo", SHORTCUT_CONTEXT_PLAYLIST);
    
    // Queue management
    registerKeyboardShortcut("add_to_queue", QKeySequence(Qt::CTRL | Qt::Key_Q), "Add to queue", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("clear_queue", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q), "Clear queue", SHORTCUT_CONTEXT_PLAYLIST);
    registerKeyboardShortcut("show_queue", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Q), "Show queue", SHORTCUT_CONTEXT_PLAYLIST);
    
    // Accessibility shortcuts
    registerKeyboardShortcut("toggle_verbosity", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V), "Toggle verbosity", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("mute_announcements", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M), "Mute announcements", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("unmute_announcements", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U), "Unmute announcements", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("repeat_last", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A), "Repeat last announcement", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("where_am_i", QKeySequence(Qt::CTRL | Qt::Key_Question), "Where am I", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("whats_this", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Question), "What's this", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("help_context", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Question), "Help context", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("accessibility_settings", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Comma), "Accessibility settings", SHORTCUT_CONTEXT_GLOBAL);
    registerKeyboardShortcut("keyboard_shortcuts", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Comma), "Keyboard shortcuts", SHORTCUT_CONTEXT_GLOBAL);
    
    logDebug("Default keyboard shortcuts registered");
}