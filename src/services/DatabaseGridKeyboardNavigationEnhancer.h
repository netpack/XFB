#ifndef DATABASEGRIDKEYBOARDNAVIGATIONENHANCER_H
#define DATABASEGRIDKEYBOARDNAVIGATIONENHANCER_H

#include <QObject>
#include <QWidget>
#include <QTableView>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QKeyEvent>
#include <QClipboard>
#include <QMimeData>
#include <QHeaderView>
#include <QSortFilterProxyModel>

class KeyboardNavigationController;
class AccessibilityManager;
class player;

/**
 * @brief Enhances database grids with comprehensive keyboard navigation and accessibility
 * 
 * This class provides specialized keyboard navigation enhancements for XFB's database
 * grids (music library, jingles, programs, etc.), including accessible grid navigation,
 * sorting, filtering, and clipboard operations as alternatives to drag-and-drop.
 * 
 * Key features:
 * - Accessible grid navigation with row/column announcements
 * - Keyboard shortcuts for sorting, filtering, and selection
 * - Cut/copy/paste operations as drag-and-drop alternatives
 * - Context-aware cell content announcements
 * - Integration with ORCA screen reader for grid navigation
 * 
 * @see KeyboardNavigationController, AccessibilityManager
 * @since XFB 2.0
 */
class DatabaseGridKeyboardNavigationEnhancer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Grid operation types for keyboard shortcuts
     */
    enum class GridOperation {
        AddToPlaylist,      ///< Add selected items to playlist
        RemoveFromGrid,     ///< Remove selected items from grid
        EditItem,           ///< Edit selected item
        SortAscending,      ///< Sort current column ascending
        SortDescending,     ///< Sort current column descending
        FilterByValue,      ///< Filter by current cell value
        ClearFilter,        ///< Clear all filters
        SelectAll,          ///< Select all items
        CopyItems,          ///< Copy selected items to clipboard
        CutItems,           ///< Cut selected items to clipboard
        PasteItems          ///< Paste items from clipboard
    };

    explicit DatabaseGridKeyboardNavigationEnhancer(player* playerWindow,
                                                   KeyboardNavigationController* navigationController,
                                                   AccessibilityManager* accessibilityManager,
                                                   QObject* parent = nullptr);
    ~DatabaseGridKeyboardNavigationEnhancer() override;

    /**
     * @brief Initialize the database grid keyboard navigation enhancements
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the database grid keyboard navigation enhancements
     */
    void shutdown();

public slots:
    /**
     * @brief Handle keyboard shortcut execution for database operations
     * @param action Action identifier
     * @param sequence Key sequence that triggered the action
     */
    void onShortcutTriggered(const QString& action, const QKeySequence& sequence);

    /**
     * @brief Handle grid navigation changes for announcements
     * @param widget Grid widget
     * @param row Current row
     * @param column Current column
     * @param itemText Current item text
     */
    void onGridNavigationChanged(QWidget* widget, int row, int column, const QString& itemText);

private slots:
    /**
     * @brief Handle selection changes in grids
     * @param selected Selected items
     * @param deselected Deselected items
     */
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    /**
     * @brief Handle header section clicks for sorting
     * @param logicalIndex Column index that was clicked
     */
    void onHeaderSectionClicked(int logicalIndex);

private:
    /**
     * @brief Setup keyboard shortcuts for database operations
     */
    void setupDatabaseShortcuts();

    /**
     * @brief Setup navigation for database grid widgets
     */
    void setupDatabaseGridNavigation();

    /**
     * @brief Setup enhanced grid accessibility features
     */
    void setupGridAccessibilityFeatures();

    /**
     * @brief Get all database grid views
     * @return List of table views used for database display
     */
    QList<QTableView*> getDatabaseGrids() const;

    /**
     * @brief Get the currently focused grid
     * @return Pointer to focused grid, or nullptr if none
     */
    QTableView* getCurrentGrid() const;

    /**
     * @brief Perform grid operation on current selection
     * @param operation Operation to perform
     * @param grid Target grid (optional, uses current if null)
     */
    void performGridOperation(GridOperation operation, QTableView* grid = nullptr);

    /**
     * @brief Add selected items to playlist
     * @param grid Source grid
     */
    void addSelectedToPlaylist(QTableView* grid);

    /**
     * @brief Remove selected items from grid
     * @param grid Target grid
     */
    void removeSelectedFromGrid(QTableView* grid);

    /**
     * @brief Edit the currently selected item
     * @param grid Target grid
     */
    void editSelectedItem(QTableView* grid);

    /**
     * @brief Sort grid by current column
     * @param grid Target grid
     * @param ascending true for ascending, false for descending
     */
    void sortGridByCurrentColumn(QTableView* grid, bool ascending);

    /**
     * @brief Filter grid by current cell value
     * @param grid Target grid
     */
    void filterGridByCurrentValue(QTableView* grid);

    /**
     * @brief Clear all filters on grid
     * @param grid Target grid
     */
    void clearGridFilters(QTableView* grid);

    /**
     * @brief Copy selected items to clipboard
     * @param grid Source grid
     */
    void copySelectedItems(QTableView* grid);

    /**
     * @brief Cut selected items to clipboard
     * @param grid Source grid
     */
    void cutSelectedItems(QTableView* grid);

    /**
     * @brief Paste items from clipboard
     * @param grid Target grid
     */
    void pasteItems(QTableView* grid);

    /**
     * @brief Get grid context information for announcements
     * @param grid Grid widget
     * @param index Model index
     * @return Formatted context string
     */
    QString getGridContextInfo(QTableView* grid, const QModelIndex& index) const;

    /**
     * @brief Get column header text
     * @param grid Grid widget
     * @param column Column index
     * @return Header text
     */
    QString getColumnHeaderText(QTableView* grid, int column) const;

    /**
     * @brief Get row information for announcements
     * @param grid Grid widget
     * @param row Row index
     * @return Formatted row information
     */
    QString getRowInfo(QTableView* grid, int row) const;

    /**
     * @brief Announce grid cell information with context
     * @param grid Grid widget
     * @param index Model index of current cell
     */
    void announceGridCellWithContext(QTableView* grid, const QModelIndex& index);

    /**
     * @brief Announce grid operation result
     * @param operation Operation that was performed
     * @param success Whether operation was successful
     * @param details Additional details about the operation
     */
    void announceGridOperation(GridOperation operation, bool success, const QString& details = QString());

    /**
     * @brief Get selection count for announcements
     * @param grid Grid widget
     * @return Formatted selection count string
     */
    QString getSelectionCountString(QTableView* grid) const;

    /**
     * @brief Check if grid supports the specified operation
     * @param grid Grid widget
     * @param operation Operation to check
     * @return true if operation is supported
     */
    bool supportsOperation(QTableView* grid, GridOperation operation) const;

    /**
     * @brief Get grid type name for announcements
     * @param grid Grid widget
     * @return Human-readable grid type name
     */
    QString getGridTypeName(QTableView* grid) const;

    // Member variables
    player* m_playerWindow;
    KeyboardNavigationController* m_navigationController;
    AccessibilityManager* m_accessibilityManager;
    
    // State tracking
    bool m_initialized;
    QTableView* m_lastFocusedGrid;
    QModelIndex m_lastSelectedIndex;
    
    // Grid references (cached for performance)
    QTableView* m_musicView;
    QTableView* m_jinglesView;
    QTableView* m_pubView;
    QTableView* m_programsView;
    
    // Clipboard data for cut/copy/paste operations
    QMimeData* m_clipboardData;
    GridOperation m_lastClipboardOperation;
    QString m_clipboardSourceGrid;
};

Q_DECLARE_METATYPE(DatabaseGridKeyboardNavigationEnhancer::GridOperation)

#endif // DATABASEGRIDKEYBOARDNAVIGATIONENHANCER_H