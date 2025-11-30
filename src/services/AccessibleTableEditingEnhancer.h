#ifndef ACCESSIBLETABLEEDITINGENHANCER_H
#define ACCESSIBLETABLEEDITINGENHANCER_H

#include <QObject>
#include <QTableView>
#include <QModelIndex>
#include <QKeyEvent>
#include <QTimer>
#include <QHash>

class AccessibilityManager;
class AccessibleTableInterface;

/**
 * @brief Enhances table editing accessibility with state announcements and keyboard shortcuts
 * 
 * This class provides comprehensive accessibility support for table editing operations,
 * including:
 * - Accessible inline editing with clear state announcements
 * - Keyboard shortcuts for edit mode entry and exit
 * - Confirmation announcements for edit operations
 * - Edit state tracking and management
 * 
 * @since XFB 2.0
 */
class AccessibleTableEditingEnhancer : public QObject
{
    Q_OBJECT

public:
    explicit AccessibleTableEditingEnhancer(QTableView* tableView, AccessibilityManager* manager, QObject* parent = nullptr);
    ~AccessibleTableEditingEnhancer() override;

    /**
     * @brief Initialize the editing enhancer
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the editing enhancer
     */
    void shutdown();

    /**
     * @brief Check if a cell is currently being edited
     * @param row The row index
     * @param column The column index
     * @return true if the cell is being edited
     */
    bool isCellBeingEdited(int row, int column) const;

    /**
     * @brief Get the currently edited cell
     * @return Model index of the currently edited cell, or invalid index if none
     */
    QModelIndex currentlyEditedCell() const { return m_currentEditIndex; }

    /**
     * @brief Enter edit mode for the current cell
     * @return true if edit mode was successfully entered
     */
    bool enterEditMode();

    /**
     * @brief Exit edit mode and save changes
     * @return true if edit mode was successfully exited
     */
    bool exitEditMode();

    /**
     * @brief Cancel edit mode without saving changes
     * @return true if edit mode was successfully cancelled
     */
    bool cancelEditMode();

protected:
    /**
     * @brief Event filter for handling keyboard shortcuts and edit events
     * @param obj The object that received the event
     * @param event The event
     * @return true if the event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief Handle selection changes in the table
     * @param current The newly selected index
     * @param previous The previously selected index
     */
    void onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

    /**
     * @brief Handle data changes in the model
     * @param topLeft The top-left index of the changed area
     * @param bottomRight The bottom-right index of the changed area
     * @param roles The roles that changed
     */
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

    /**
     * @brief Handle edit triggers from the view
     * @param index The index being edited
     */
    void onEditTriggered(const QModelIndex& index);

    /**
     * @brief Delayed announcement processing
     */
    void processDelayedAnnouncement();

private:
    /**
     * @brief Handle keyboard shortcuts for editing
     * @param event The key event
     * @return true if the event was handled
     */
    bool handleEditingKeyboard(QKeyEvent* event);

    /**
     * @brief Announce edit state change
     * @param entering true if entering edit mode, false if exiting
     * @param index The cell index being edited
     */
    void announceEditStateChange(bool entering, const QModelIndex& index);

    /**
     * @brief Announce edit confirmation
     * @param index The cell that was edited
     * @param oldValue The previous value
     * @param newValue The new value
     */
    void announceEditConfirmation(const QModelIndex& index, const QString& oldValue, const QString& newValue);

    /**
     * @brief Get cell value as string
     * @param index The cell index
     * @return String representation of the cell value
     */
    QString getCellValue(const QModelIndex& index) const;

    /**
     * @brief Get column name for announcements
     * @param column The column index
     * @return Column name or generic description
     */
    QString getColumnName(int column) const;

    /**
     * @brief Check if the current cell is editable
     * @param index The cell index
     * @return true if the cell is editable
     */
    bool isCellEditable(const QModelIndex& index) const;

    /**
     * @brief Setup keyboard shortcuts for editing
     */
    void setupKeyboardShortcuts();

    /**
     * @brief Connect to table view signals
     */
    void connectTableSignals();

    /**
     * @brief Disconnect from table view signals
     */
    void disconnectTableSignals();

    // Member variables
    QTableView* m_tableView;
    AccessibilityManager* m_accessibilityManager;
    AccessibleTableInterface* m_tableInterface;
    
    // Edit state tracking
    QModelIndex m_currentEditIndex;
    QString m_originalValue;
    bool m_isEditing;
    bool m_editingEnabled;
    
    // Announcement management
    QTimer* m_announcementTimer;
    QString m_pendingAnnouncement;
    
    // Value tracking for change detection
    QHash<QModelIndex, QString> m_previousValues;
    
    // Configuration
    static constexpr int ANNOUNCEMENT_DELAY_MS = 150;
};

#endif // ACCESSIBLETABLEEDITINGENHANCER_H