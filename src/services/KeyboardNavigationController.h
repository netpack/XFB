#ifndef KEYBOARDNAVIGATIONCONTROLLER_H
#define KEYBOARDNAVIGATIONCONTROLLER_H

#include <QObject>
#include <QWidget>
#include <QKeyEvent>
#include <QKeySequence>
#include <QHash>
#include <QList>
#include <QTimer>
#include <QApplication>
#include <QTableView>
#include <QListWidget>
#include <QAbstractItemView>
#include <QFocusFrame>

class AccessibilityManager;

/**
 * @brief Controller for comprehensive keyboard navigation throughout XFB
 * 
 * The KeyboardNavigationController provides application-wide keyboard navigation
 * support, including custom tab order management, arrow key navigation for complex
 * widgets, and keyboard shortcuts for all major functions. This class is essential
 * for accessibility compliance and enables full keyboard-only operation.
 * 
 * Key features:
 * - Logical tab order management that follows visual flow
 * - Arrow key navigation within grids, lists, and complex controls
 * - Keyboard shortcut registration and conflict resolution
 * - Focus management with accessibility announcements
 * - Grid and table navigation with proper context
 * 
 * @example
 * @code
 * KeyboardNavigationController* controller = new KeyboardNavigationController(accessibilityManager);
 * controller->initialize();
 * controller->registerKeyboardShortcut("play_pause", QKeySequence(Qt::Key_Space), "Toggle playback");
 * controller->setCustomTabOrder(widget1, widget2);
 * @endcode
 * 
 * @see AccessibilityManager
 * @since XFB 2.0
 */
class KeyboardNavigationController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Navigation context for different widget types
     */
    enum class NavigationContext {
        General,        ///< General application navigation
        Grid,          ///< Table/grid navigation with row/column support
        List,          ///< List navigation with item selection
        Player,        ///< Media player controls
        Playlist,      ///< Playlist management
        Database       ///< Database grid operations
    };

    /**
     * @brief Keyboard shortcut information
     */
    struct KeyboardShortcut {
        QString action;           ///< Action identifier
        QKeySequence sequence;    ///< Key sequence
        QString description;      ///< Human-readable description
        QString context;          ///< Context where shortcut is active
        bool customizable;        ///< Whether user can customize this shortcut
        QWidget* targetWidget;    ///< Widget that should receive the action (optional)
        
        KeyboardShortcut() : customizable(true), targetWidget(nullptr) {}
    };

    /**
     * @brief Focus navigation information
     */
    struct FocusNavigationInfo {
        QWidget* widget;
        NavigationContext context;
        QString accessibleName;
        QString navigationHint;
        
        FocusNavigationInfo() : widget(nullptr), context(NavigationContext::General) {}
    };

    explicit KeyboardNavigationController(AccessibilityManager* accessibilityManager, QObject* parent = nullptr);
    ~KeyboardNavigationController() override;

    /**
     * @brief Initialize the keyboard navigation system
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the keyboard navigation system
     */
    void shutdown();

    /**
     * @brief Register a keyboard shortcut
     * @param action Action identifier
     * @param sequence Key sequence
     * @param description Human-readable description
     * @param context Context where shortcut is active (optional)
     * @param customizable Whether user can customize this shortcut
     * @param targetWidget Widget that should receive the action (optional)
     * @return true if registration was successful
     */
    bool registerKeyboardShortcut(const QString& action, 
                                 const QKeySequence& sequence,
                                 const QString& description,
                                 const QString& context = QString(),
                                 bool customizable = true,
                                 QWidget* targetWidget = nullptr);

    /**
     * @brief Unregister a keyboard shortcut
     * @param action Action identifier to unregister
     */
    void unregisterKeyboardShortcut(const QString& action);

    /**
     * @brief Get all registered keyboard shortcuts
     * @return Hash map of action identifiers to shortcut information
     */
    QHash<QString, KeyboardShortcut> getKeyboardShortcuts() const { return m_shortcuts; }

    /**
     * @brief Get keyboard shortcut by action
     * @param action Action identifier
     * @return Shortcut information, or default if not found
     */
    KeyboardShortcut getKeyboardShortcut(const QString& action) const;

    /**
     * @brief Set custom tab order between widgets
     * @param first First widget in tab order
     * @param second Second widget in tab order
     */
    void setCustomTabOrder(QWidget* first, QWidget* second);

    /**
     * @brief Register a widget for enhanced keyboard navigation
     * @param widget Widget to register
     * @param context Navigation context for the widget
     * @param accessibleName Accessible name for announcements
     * @param navigationHint Optional navigation hint for users
     */
    void registerNavigationWidget(QWidget* widget, 
                                 NavigationContext context,
                                 const QString& accessibleName,
                                 const QString& navigationHint = QString());

    /**
     * @brief Unregister a widget from keyboard navigation
     * @param widget Widget to unregister
     */
    void unregisterNavigationWidget(QWidget* widget);

    /**
     * @brief Enable or disable arrow key navigation for a widget
     * @param widget Widget to configure
     * @param enabled Whether to enable arrow key navigation
     */
    void setArrowKeyNavigationEnabled(QWidget* widget, bool enabled);

    /**
     * @brief Check if arrow key navigation is enabled for a widget
     * @param widget Widget to check
     * @return true if arrow key navigation is enabled
     */
    bool isArrowKeyNavigationEnabled(QWidget* widget) const;

    /**
     * @brief Get navigation context for a widget
     * @param widget Widget to check
     * @return Navigation context, or General if not registered
     */
    NavigationContext getNavigationContext(QWidget* widget) const;

    /**
     * @brief Announce current focus information
     * @param widget Currently focused widget
     */
    void announceFocusInfo(QWidget* widget);

public slots:
    /**
     * @brief Handle focus change events
     * @param old Previously focused widget
     * @param now Currently focused widget
     */
    void onFocusChanged(QWidget* old, QWidget* now);

    /**
     * @brief Execute a keyboard shortcut action
     * @param action Action identifier to execute
     */
    void executeShortcutAction(const QString& action);

signals:
    /**
     * @brief Emitted when a keyboard shortcut is triggered
     * @param action Action identifier
     * @param sequence Key sequence that triggered the action
     */
    void shortcutTriggered(const QString& action, const QKeySequence& sequence);

    /**
     * @brief Emitted when navigation context changes
     * @param widget Currently focused widget
     * @param context New navigation context
     */
    void navigationContextChanged(QWidget* widget, NavigationContext context);

    /**
     * @brief Emitted when focus moves within a grid or list
     * @param widget Grid or list widget
     * @param row Current row
     * @param column Current column (for grids)
     * @param itemText Current item text
     */
    void gridNavigationChanged(QWidget* widget, int row, int column, const QString& itemText);

protected:
    /**
     * @brief Event filter for handling keyboard events
     * @param obj Object that received the event
     * @param event Event to process
     * @return true if event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief Handle widget destruction
     * @param obj Destroyed widget
     */
    void onWidgetDestroyed(QObject* obj);

    /**
     * @brief Process delayed focus announcements
     */
    void processFocusAnnouncement();

private:
    /**
     * @brief Handle key press events
     * @param widget Widget that received the key press
     * @param event Key event
     * @return true if event was handled
     */
    bool handleKeyPress(QWidget* widget, QKeyEvent* event);

    /**
     * @brief Handle arrow key navigation
     * @param widget Widget receiving arrow key
     * @param key Arrow key pressed
     * @return true if navigation was handled
     */
    bool handleArrowKeyNavigation(QWidget* widget, Qt::Key key);

    /**
     * @brief Handle grid navigation (tables, tree views)
     * @param view Item view widget
     * @param key Arrow key pressed
     * @return true if navigation was handled
     */
    bool handleGridNavigation(QAbstractItemView* view, Qt::Key key);

    /**
     * @brief Handle list navigation
     * @param list List widget
     * @param key Arrow key pressed
     * @return true if navigation was handled
     */
    bool handleListNavigation(QListWidget* list, Qt::Key key);

    /**
     * @brief Handle tab navigation
     * @param widget Current widget
     * @param forward true for Tab, false for Shift+Tab
     * @return true if navigation was handled
     */
    bool handleTabNavigation(QWidget* widget, bool forward);

    /**
     * @brief Check if a key sequence conflicts with existing shortcuts
     * @param sequence Key sequence to check
     * @param excludeAction Action to exclude from conflict check
     * @return true if there's a conflict
     */
    bool hasShortcutConflict(const QKeySequence& sequence, const QString& excludeAction = QString()) const;

    /**
     * @brief Install event filter on a widget and its children
     * @param widget Widget to install filter on
     */
    void installEventFilterRecursively(QWidget* widget);

    /**
     * @brief Remove event filter from a widget and its children
     * @param widget Widget to remove filter from
     */
    void removeEventFilterRecursively(QWidget* widget);

    /**
     * @brief Setup default keyboard shortcuts
     */
    void setupDefaultShortcuts();

    /**
     * @brief Get grid position information for announcements
     * @param view Item view widget
     * @param index Current model index
     * @return Formatted position string
     */
    QString getGridPositionInfo(QAbstractItemView* view, const QModelIndex& index);

    /**
     * @brief Get list position information for announcements
     * @param list List widget
     * @param row Current row
     * @return Formatted position string
     */
    QString getListPositionInfo(QListWidget* list, int row);

    /**
     * @brief Announce grid cell information
     * @param view Item view widget
     * @param index Model index of current cell
     */
    void announceGridCell(QAbstractItemView* view, const QModelIndex& index);

    /**
     * @brief Announce list item information
     * @param list List widget
     * @param row Current row
     */
    void announceListItem(QListWidget* list, int row);

    // Member variables
    AccessibilityManager* m_accessibilityManager;
    
    // Keyboard shortcuts management
    QHash<QString, KeyboardShortcut> m_shortcuts;
    QHash<QKeySequence, QString> m_sequenceToAction;
    
    // Widget navigation management
    QHash<QWidget*, FocusNavigationInfo> m_navigationWidgets;
    QHash<QWidget*, bool> m_arrowKeyEnabled;
    
    // Tab order management
    QHash<QWidget*, QWidget*> m_customTabOrder;
    
    // Focus management
    QWidget* m_lastFocusedWidget;
    QTimer* m_focusAnnouncementTimer;
    QString m_pendingFocusAnnouncement;
    
    // State tracking
    bool m_initialized;
    bool m_eventFilterInstalled;
    
    // Configuration constants
    static constexpr int FOCUS_ANNOUNCEMENT_DELAY_MS = 150;
    static constexpr const char* SHORTCUT_CONTEXT_GLOBAL = "Global";
    static constexpr const char* SHORTCUT_CONTEXT_PLAYER = "Player";
    static constexpr const char* SHORTCUT_CONTEXT_DATABASE = "Database";
    static constexpr const char* SHORTCUT_CONTEXT_PLAYLIST = "Playlist";
};

Q_DECLARE_METATYPE(KeyboardNavigationController::NavigationContext)

#endif // KEYBOARDNAVIGATIONCONTROLLER_H