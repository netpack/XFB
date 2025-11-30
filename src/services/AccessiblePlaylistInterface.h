#ifndef ACCESSIBLEPLAYLISTINTERFACE_H
#define ACCESSIBLEPLAYLISTINTERFACE_H

#include <QAccessibleInterface>
#include <QListWidget>
#include <QModelIndex>
#include <QKeyEvent>
#include <QTimer>

class AccessibilityManager;

/**
 * @brief Custom accessible interface for QListWidget playlist widgets
 * 
 * This class provides comprehensive accessibility support for playlist widgets,
 * including:
 * - Accessible list navigation with proper announcements
 * - Drag-and-drop alternatives with keyboard operations
 * - Playlist modification announcements and feedback
 * - Item reordering with keyboard shortcuts
 * 
 * @since XFB 2.0
 */
class AccessiblePlaylistInterface : public QAccessibleInterface
{
public:
    explicit AccessiblePlaylistInterface(QListWidget* listWidget, AccessibilityManager* manager = nullptr);
    ~AccessiblePlaylistInterface() override;

    // QAccessibleInterface implementation
    bool isValid() const override;
    QObject* object() const override;
    QWindow* window() const override;
    QAccessibleInterface* parent() const override;
    QAccessibleInterface* child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString& text) override;
    QRect rect() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QColor foregroundColor() const override;
    QColor backgroundColor() const override;
    QAccessibleInterface* focusChild() const override;
    QAccessibleInterface* childAt(int x, int y) const override;

    /**
     * @brief Get the list widget
     * @return Pointer to the list widget
     */
    QListWidget* listWidget() const { return m_listWidget; }

    /**
     * @brief Announce current item context
     * @param row The item row index
     */
    void announceItemContext(int row);

    /**
     * @brief Announce playlist modification
     * @param operation The operation performed (added, removed, moved, etc.)
     * @param itemText The text of the affected item
     * @param position Optional position information
     */
    void announcePlaylistModification(const QString& operation, const QString& itemText, const QString& position = QString());

private:
    /**
     * @brief Get formatted item content with context
     * @param row The item row index
     * @return Formatted item content string
     */
    QString getItemContentWithContext(int row) const;

    /**
     * @brief Get current selection information
     * @return Formatted selection information string
     */
    QString getSelectionInfo() const;

    QListWidget* m_listWidget;
    AccessibilityManager* m_accessibilityManager;
    mutable QHash<int, QAccessibleInterface*> m_itemInterfaces;
};

/**
 * @brief Accessible interface for individual playlist items
 */
class AccessiblePlaylistItemInterface : public QAccessibleInterface
{
public:
    AccessiblePlaylistItemInterface(QListWidget* listWidget, int row, AccessiblePlaylistInterface* parent);
    ~AccessiblePlaylistItemInterface() override;

    // QAccessibleInterface implementation
    bool isValid() const override;
    QObject* object() const override;
    QWindow* window() const override;
    QAccessibleInterface* parent() const override;
    QAccessibleInterface* child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString& text) override;
    QRect rect() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QColor foregroundColor() const override;
    QColor backgroundColor() const override;
    QAccessibleInterface* focusChild() const override;
    QAccessibleInterface* childAt(int x, int y) const override;

    /**
     * @brief Get the row index of this item
     * @return Row index
     */
    int row() const { return m_row; }

private:
    QListWidget* m_listWidget;
    AccessiblePlaylistInterface* m_parentInterface;
    int m_row;
};

/**
 * @brief Enhances playlist accessibility with keyboard operations and drag-drop alternatives
 */
class AccessiblePlaylistEnhancer : public QObject
{
    Q_OBJECT

public:
    explicit AccessiblePlaylistEnhancer(QListWidget* listWidget, AccessibilityManager* manager, QObject* parent = nullptr);
    ~AccessiblePlaylistEnhancer() override;

    /**
     * @brief Initialize the playlist enhancer
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the playlist enhancer
     */
    void shutdown();

    /**
     * @brief Move selected item up in the playlist
     * @return true if the item was moved
     */
    bool moveItemUp();

    /**
     * @brief Move selected item down in the playlist
     * @return true if the item was moved
     */
    bool moveItemDown();

    /**
     * @brief Cut selected item (for keyboard-based move operations)
     * @return true if the item was cut
     */
    bool cutItem();

    /**
     * @brief Paste cut item at current position
     * @return true if the item was pasted
     */
    bool pasteItem();

    /**
     * @brief Delete selected item from playlist
     * @return true if the item was deleted
     */
    bool deleteItem();

protected:
    /**
     * @brief Event filter for handling keyboard shortcuts
     * @param obj The object that received the event
     * @param event The event
     * @return true if the event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief Handle current item changes
     * @param current The newly selected item
     * @param previous The previously selected item
     */
    void onCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

    /**
     * @brief Handle item selection changes
     */
    void onItemSelectionChanged();

    /**
     * @brief Handle model changes (items added/removed)
     */
    void onModelChanged();

    /**
     * @brief Process delayed announcements
     */
    void processDelayedAnnouncement();

private:
    /**
     * @brief Handle keyboard shortcuts for playlist operations
     * @param event The key event
     * @return true if the event was handled
     */
    bool handlePlaylistKeyboard(QKeyEvent* event);

    /**
     * @brief Announce item context with delay
     * @param item The item to announce
     */
    void announceItemWithDelay(QListWidgetItem* item);

    /**
     * @brief Get item position description
     * @param row The item row
     * @return Position description string
     */
    QString getItemPositionDescription(int row) const;

    /**
     * @brief Connect to list widget signals
     */
    void connectListSignals();

    /**
     * @brief Disconnect from list widget signals
     */
    void disconnectListSignals();

    // Member variables
    QListWidget* m_listWidget;
    AccessibilityManager* m_accessibilityManager;
    AccessiblePlaylistInterface* m_playlistInterface;
    
    // Cut/paste operations
    QListWidgetItem* m_cutItem;
    int m_cutItemRow;
    
    // Announcement management
    QTimer* m_announcementTimer;
    QString m_pendingAnnouncement;
    
    // Configuration
    static constexpr int ANNOUNCEMENT_DELAY_MS = 200;
};

#endif // ACCESSIBLEPLAYLISTINTERFACE_H