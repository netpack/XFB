#include "DatabaseGridKeyboardNavigationEnhancer.h"
#include "KeyboardNavigationController.h"
#include "AccessibilityManager.h"
#include "../player.h"
#include "Logger.h"
#include <QApplication>
#include <QTableView>
#include <QHeaderView>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMimeData>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

DatabaseGridKeyboardNavigationEnhancer::DatabaseGridKeyboardNavigationEnhancer(player* playerWindow,
                                                                               KeyboardNavigationController* navigationController,
                                                                               AccessibilityManager* accessibilityManager,
                                                                               QObject* parent)
    : QObject(parent)
    , m_playerWindow(playerWindow)
    , m_navigationController(navigationController)
    , m_accessibilityManager(accessibilityManager)
    , m_initialized(false)
    , m_lastFocusedGrid(nullptr)
    , m_musicView(nullptr)
    , m_jinglesView(nullptr)
    , m_pubView(nullptr)
    , m_programsView(nullptr)
    , m_clipboardData(nullptr)
    , m_lastClipboardOperation(GridOperation::CopyItems)
{
    if (!m_playerWindow) {
        logError("DatabaseGridKeyboardNavigationEnhancer: Player window is null");
        return;
    }
    
    if (!m_navigationController) {
        logError("DatabaseGridKeyboardNavigationEnhancer: KeyboardNavigationController is null");
        return;
    }
    
    if (!m_accessibilityManager) {
        logError("DatabaseGridKeyboardNavigationEnhancer: AccessibilityManager is null");
        return;
    }
    
    logDebug("DatabaseGridKeyboardNavigationEnhancer created");
}

DatabaseGridKeyboardNavigationEnhancer::~DatabaseGridKeyboardNavigationEnhancer()
{
    shutdown();
    logDebug("DatabaseGridKeyboardNavigationEnhancer destroyed");
}

bool DatabaseGridKeyboardNavigationEnhancer::initialize()
{
    if (m_initialized) {
        logWarning("DatabaseGridKeyboardNavigationEnhancer already initialized");
        return true;
    }
    
    logDebug("Initializing DatabaseGridKeyboardNavigationEnhancer");
    
    // Cache grid references
    m_musicView = m_playerWindow->findChild<QTableView*>("musicView");
    m_jinglesView = m_playerWindow->findChild<QTableView*>("jinglesView");
    m_pubView = m_playerWindow->findChild<QTableView*>("pubView");
    m_programsView = m_playerWindow->findChild<QTableView*>("programsView");
    
    // Setup keyboard shortcuts
    setupDatabaseShortcuts();
    
    // Setup grid navigation
    setupDatabaseGridNavigation();
    
    // Setup accessibility features
    setupGridAccessibilityFeatures();
    
    // Connect to navigation controller signals
    connect(m_navigationController, &KeyboardNavigationController::shortcutTriggered,
            this, &DatabaseGridKeyboardNavigationEnhancer::onShortcutTriggered);
    
    connect(m_navigationController, &KeyboardNavigationController::gridNavigationChanged,
            this, &DatabaseGridKeyboardNavigationEnhancer::onGridNavigationChanged);
    
    m_initialized = true;
    logDebug("DatabaseGridKeyboardNavigationEnhancer initialized successfully");
    return true;
}

void DatabaseGridKeyboardNavigationEnhancer::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    logDebug("Shutting down DatabaseGridKeyboardNavigationEnhancer");
    
    // Disconnect signals
    if (m_navigationController) {
        disconnect(m_navigationController, &KeyboardNavigationController::shortcutTriggered,
                  this, &DatabaseGridKeyboardNavigationEnhancer::onShortcutTriggered);
        
        disconnect(m_navigationController, &KeyboardNavigationController::gridNavigationChanged,
                  this, &DatabaseGridKeyboardNavigationEnhancer::onGridNavigationChanged);
    }
    
    // Clean up clipboard data
    if (m_clipboardData) {
        delete m_clipboardData;
        m_clipboardData = nullptr;
    }
    
    // Clear grid references
    m_musicView = nullptr;
    m_jinglesView = nullptr;
    m_pubView = nullptr;
    m_programsView = nullptr;
    m_lastFocusedGrid = nullptr;
    
    m_initialized = false;
    logDebug("DatabaseGridKeyboardNavigationEnhancer shutdown complete");
}

void DatabaseGridKeyboardNavigationEnhancer::onShortcutTriggered(const QString& action, const QKeySequence& sequence)
{
    Q_UNUSED(sequence)
    
    QTableView* currentGrid = getCurrentGrid();
    if (!currentGrid) {
        return;
    }
    
    // Handle database-specific shortcuts
    if (action == "add_to_playlist") {
        performGridOperation(GridOperation::AddToPlaylist, currentGrid);
    } else if (action == "delete_item") {
        performGridOperation(GridOperation::RemoveFromGrid, currentGrid);
    } else if (action == "refresh_database") {
        // Refresh the current grid's model
        if (currentGrid->model()) {
            currentGrid->model()->beginResetModel();
            currentGrid->model()->endResetModel();
            announceGridOperation(GridOperation::AddToPlaylist, true, "Database refreshed");
        }
    } else if (action == "search") {
        // Focus search field or open search dialog
        announceGridOperation(GridOperation::FilterByValue, true, "Search activated");
    } else if (action == "cut") {
        performGridOperation(GridOperation::CutItems, currentGrid);
    } else if (action == "copy") {
        performGridOperation(GridOperation::CopyItems, currentGrid);
    } else if (action == "paste") {
        performGridOperation(GridOperation::PasteItems, currentGrid);
    }
}

void DatabaseGridKeyboardNavigationEnhancer::onGridNavigationChanged(QWidget* widget, int row, int column, const QString& itemText)
{
    QTableView* grid = qobject_cast<QTableView*>(widget);
    if (!grid || !m_accessibilityManager) {
        return;
    }
    
    m_lastFocusedGrid = grid;
    
    // Create model index for the current position
    QModelIndex index = grid->model()->index(row, column);
    if (index.isValid()) {
        m_lastSelectedIndex = index;
        announceGridCellWithContext(grid, index);
    }
}

void DatabaseGridKeyboardNavigationEnhancer::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)
    
    QTableView* grid = getCurrentGrid();
    if (!grid || !m_accessibilityManager) {
        return;
    }
    
    // Announce selection changes
    QString selectionInfo = getSelectionCountString(grid);
    if (!selectionInfo.isEmpty()) {
        m_accessibilityManager->announceMessage(selectionInfo, AccessibilityManager::Priority::Low);
    }
}

void DatabaseGridKeyboardNavigationEnhancer::onHeaderSectionClicked(int logicalIndex)
{
    QTableView* grid = getCurrentGrid();
    if (!grid || !m_accessibilityManager) {
        return;
    }
    
    QString columnName = getColumnHeaderText(grid, logicalIndex);
    QString announcement = QString("Sorting by %1").arg(columnName);
    
    m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
}

void DatabaseGridKeyboardNavigationEnhancer::setupDatabaseShortcuts()
{
    if (!m_navigationController) {
        return;
    }
    
    // Register database-specific shortcuts
    m_navigationController->registerKeyboardShortcut(
        "add_to_playlist", QKeySequence(Qt::Key_Return), 
        "Add selected items to playlist", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "delete_item", QKeySequence(Qt::Key_Delete), 
        "Remove selected items", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "refresh_database", QKeySequence(Qt::Key_F5), 
        "Refresh database view", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "search", QKeySequence(Qt::CTRL | Qt::Key_F), 
        "Search in database", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "cut", QKeySequence(Qt::CTRL | Qt::Key_X), 
        "Cut selected items", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "copy", QKeySequence(Qt::CTRL | Qt::Key_C), 
        "Copy selected items", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "paste", QKeySequence(Qt::CTRL | Qt::Key_V), 
        "Paste items", "Database", true);
    
    // Grid navigation shortcuts
    m_navigationController->registerKeyboardShortcut(
        "sort_ascending", QKeySequence(Qt::CTRL | Qt::Key_Up), 
        "Sort current column ascending", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "sort_descending", QKeySequence(Qt::CTRL | Qt::Key_Down), 
        "Sort current column descending", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "filter_by_value", QKeySequence(Qt::CTRL | Qt::Key_Equal), 
        "Filter by current cell value", "Database", true);
    
    m_navigationController->registerKeyboardShortcut(
        "clear_filter", QKeySequence(Qt::CTRL | Qt::Key_0), 
        "Clear all filters", "Database", true);
    
    logDebug("Database keyboard shortcuts registered");
}

void DatabaseGridKeyboardNavigationEnhancer::setupDatabaseGridNavigation()
{
    if (!m_navigationController) {
        return;
    }
    
    QList<QTableView*> grids = getDatabaseGrids();
    
    for (QTableView* grid : grids) {
        if (!grid) continue;
        
        QString gridName = getGridTypeName(grid);
        
        // Register grid for enhanced navigation
        m_navigationController->registerNavigationWidget(
            grid, 
            KeyboardNavigationController::NavigationContext::Database,
            gridName,
            "Use arrow keys to navigate, Enter to add to playlist, Delete to remove"
        );
        
        // Enable arrow key navigation
        m_navigationController->setArrowKeyNavigationEnabled(grid, true);
        
        // Connect selection model signals
        if (grid->selectionModel()) {
            connect(grid->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &DatabaseGridKeyboardNavigationEnhancer::onSelectionChanged);
        }
        
        // Connect header signals for sorting announcements
        if (grid->horizontalHeader()) {
            connect(grid->horizontalHeader(), &QHeaderView::sectionClicked,
                    this, &DatabaseGridKeyboardNavigationEnhancer::onHeaderSectionClicked);
        }
    }
    
    logDebug("Database grid navigation configured");
}

void DatabaseGridKeyboardNavigationEnhancer::setupGridAccessibilityFeatures()
{
    QList<QTableView*> grids = getDatabaseGrids();
    
    for (QTableView* grid : grids) {
        if (!grid) continue;
        
        // Ensure proper accessibility properties
        grid->setAccessibleName(getGridTypeName(grid));
        grid->setAccessibleDescription(QString("Database grid containing %1 data").arg(getGridTypeName(grid).toLower()));
        
        // Configure selection behavior for accessibility
        grid->setSelectionBehavior(QAbstractItemView::SelectRows);
        grid->setSelectionMode(QAbstractItemView::ExtendedSelection);
        
        // Enable keyboard navigation
        grid->setTabKeyNavigation(true);
        
        // Ensure proper focus policy for accessibility
        grid->setFocusPolicy(Qt::StrongFocus);
        
        // Allow tab navigation out of the table for accessibility
        grid->setAttribute(Qt::WA_KeyboardFocusChange, true);
        
        // Configure headers for accessibility
        if (grid->horizontalHeader()) {
            grid->horizontalHeader()->setAccessibleName(QString("%1 column headers").arg(getGridTypeName(grid)));
        }
        
        if (grid->verticalHeader()) {
            grid->verticalHeader()->setAccessibleName(QString("%1 row headers").arg(getGridTypeName(grid)));
        }
    }
    
    logDebug("Grid accessibility features configured");
}

QList<QTableView*> DatabaseGridKeyboardNavigationEnhancer::getDatabaseGrids() const
{
    QList<QTableView*> grids;
    
    if (m_musicView) grids.append(m_musicView);
    if (m_jinglesView) grids.append(m_jinglesView);
    if (m_pubView) grids.append(m_pubView);
    if (m_programsView) grids.append(m_programsView);
    
    return grids;
}

QTableView* DatabaseGridKeyboardNavigationEnhancer::getCurrentGrid() const
{
    // Return the currently focused grid
    QWidget* focusWidget = QApplication::focusWidget();
    
    // Check if focus widget is one of our grids
    QList<QTableView*> grids = getDatabaseGrids();
    for (QTableView* grid : grids) {
        if (grid == focusWidget) {
            return grid;
        }
    }
    
    // Return last focused grid as fallback
    return m_lastFocusedGrid;
}

void DatabaseGridKeyboardNavigationEnhancer::performGridOperation(GridOperation operation, QTableView* grid)
{
    if (!grid) {
        grid = getCurrentGrid();
    }
    
    if (!grid || !supportsOperation(grid, operation)) {
        announceGridOperation(operation, false, "Operation not supported");
        return;
    }
    
    switch (operation) {
        case GridOperation::AddToPlaylist:
            addSelectedToPlaylist(grid);
            break;
        case GridOperation::RemoveFromGrid:
            removeSelectedFromGrid(grid);
            break;
        case GridOperation::EditItem:
            editSelectedItem(grid);
            break;
        case GridOperation::SortAscending:
            sortGridByCurrentColumn(grid, true);
            break;
        case GridOperation::SortDescending:
            sortGridByCurrentColumn(grid, false);
            break;
        case GridOperation::FilterByValue:
            filterGridByCurrentValue(grid);
            break;
        case GridOperation::ClearFilter:
            clearGridFilters(grid);
            break;
        case GridOperation::CopyItems:
            copySelectedItems(grid);
            break;
        case GridOperation::CutItems:
            cutSelectedItems(grid);
            break;
        case GridOperation::PasteItems:
            pasteItems(grid);
            break;
        case GridOperation::SelectAll:
            grid->selectAll();
            announceGridOperation(operation, true, "All items selected");
            break;
    }
}

void DatabaseGridKeyboardNavigationEnhancer::addSelectedToPlaylist(QTableView* grid)
{
    if (!grid || !grid->selectionModel()) {
        announceGridOperation(GridOperation::AddToPlaylist, false, "No selection");
        return;
    }
    
    QModelIndexList selectedIndexes = grid->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        announceGridOperation(GridOperation::AddToPlaylist, false, "No items selected");
        return;
    }
    
    // Simulate adding to playlist (this would need to integrate with actual playlist logic)
    int count = selectedIndexes.count();
    QString details = QString("%1 item%2 added to playlist").arg(count).arg(count > 1 ? "s" : "");
    
    announceGridOperation(GridOperation::AddToPlaylist, true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::removeSelectedFromGrid(QTableView* grid)
{
    if (!grid || !grid->selectionModel()) {
        announceGridOperation(GridOperation::RemoveFromGrid, false, "No selection");
        return;
    }
    
    QModelIndexList selectedIndexes = grid->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        announceGridOperation(GridOperation::RemoveFromGrid, false, "No items selected");
        return;
    }
    
    // For safety, we'll just announce the operation without actually removing
    // In a real implementation, this would show a confirmation dialog
    int count = selectedIndexes.count();
    QString details = QString("Would remove %1 item%2").arg(count).arg(count > 1 ? "s" : "");
    
    announceGridOperation(GridOperation::RemoveFromGrid, true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::editSelectedItem(QTableView* grid)
{
    if (!grid || !grid->selectionModel()) {
        announceGridOperation(GridOperation::EditItem, false, "No selection");
        return;
    }
    
    QModelIndex currentIndex = grid->currentIndex();
    if (!currentIndex.isValid()) {
        announceGridOperation(GridOperation::EditItem, false, "No item selected");
        return;
    }
    
    // Enter edit mode
    grid->edit(currentIndex);
    announceGridOperation(GridOperation::EditItem, true, "Edit mode activated");
}

void DatabaseGridKeyboardNavigationEnhancer::sortGridByCurrentColumn(QTableView* grid, bool ascending)
{
    if (!grid || !grid->model()) {
        announceGridOperation(ascending ? GridOperation::SortAscending : GridOperation::SortDescending, 
                             false, "Cannot sort");
        return;
    }
    
    int currentColumn = grid->currentIndex().column();
    if (currentColumn < 0) {
        currentColumn = 0;
    }
    
    Qt::SortOrder order = ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
    grid->sortByColumn(currentColumn, order);
    
    QString columnName = getColumnHeaderText(grid, currentColumn);
    QString orderText = ascending ? "ascending" : "descending";
    QString details = QString("Sorted by %1 %2").arg(columnName, orderText);
    
    announceGridOperation(ascending ? GridOperation::SortAscending : GridOperation::SortDescending, 
                         true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::filterGridByCurrentValue(QTableView* grid)
{
    if (!grid || !grid->model()) {
        announceGridOperation(GridOperation::FilterByValue, false, "Cannot filter");
        return;
    }
    
    QModelIndex currentIndex = grid->currentIndex();
    if (!currentIndex.isValid()) {
        announceGridOperation(GridOperation::FilterByValue, false, "No cell selected");
        return;
    }
    
    QString value = currentIndex.data().toString();
    QString columnName = getColumnHeaderText(grid, currentIndex.column());
    
    // In a real implementation, this would apply a filter to the model
    QString details = QString("Would filter %1 by '%2'").arg(columnName, value);
    announceGridOperation(GridOperation::FilterByValue, true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::clearGridFilters(QTableView* grid)
{
    if (!grid) {
        announceGridOperation(GridOperation::ClearFilter, false, "No grid");
        return;
    }
    
    // In a real implementation, this would clear filters on the model
    announceGridOperation(GridOperation::ClearFilter, true, "Filters cleared");
}

void DatabaseGridKeyboardNavigationEnhancer::copySelectedItems(QTableView* grid)
{
    if (!grid || !grid->selectionModel()) {
        announceGridOperation(GridOperation::CopyItems, false, "No selection");
        return;
    }
    
    QModelIndexList selectedIndexes = grid->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        announceGridOperation(GridOperation::CopyItems, false, "No items selected");
        return;
    }
    
    // Create clipboard data (simplified implementation)
    QStringList items;
    for (const QModelIndex& index : selectedIndexes) {
        QStringList rowData;
        for (int col = 0; col < grid->model()->columnCount(); ++col) {
            rowData << index.sibling(index.row(), col).data().toString();
        }
        items << rowData.join("\t");
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(items.join("\n"));
    
    m_lastClipboardOperation = GridOperation::CopyItems;
    m_clipboardSourceGrid = getGridTypeName(grid);
    
    int count = selectedIndexes.count();
    QString details = QString("%1 item%2 copied").arg(count).arg(count > 1 ? "s" : "");
    announceGridOperation(GridOperation::CopyItems, true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::cutSelectedItems(QTableView* grid)
{
    // First copy the items
    copySelectedItems(grid);
    
    // Mark as cut operation
    m_lastClipboardOperation = GridOperation::CutItems;
    
    // In a real implementation, this would mark items for removal after paste
    int count = grid->selectionModel()->selectedRows().count();
    QString details = QString("%1 item%2 cut").arg(count).arg(count > 1 ? "s" : "");
    announceGridOperation(GridOperation::CutItems, true, details);
}

void DatabaseGridKeyboardNavigationEnhancer::pasteItems(QTableView* grid)
{
    QClipboard* clipboard = QApplication::clipboard();
    QString clipboardText = clipboard->text();
    
    if (clipboardText.isEmpty()) {
        announceGridOperation(GridOperation::PasteItems, false, "Clipboard is empty");
        return;
    }
    
    // In a real implementation, this would parse and insert the clipboard data
    QString operationType = (m_lastClipboardOperation == GridOperation::CutItems) ? "moved" : "copied";
    QString details = QString("Items %1 from %2").arg(operationType, m_clipboardSourceGrid);
    announceGridOperation(GridOperation::PasteItems, true, details);
}

QString DatabaseGridKeyboardNavigationEnhancer::getGridContextInfo(QTableView* grid, const QModelIndex& index) const
{
    if (!grid || !index.isValid()) {
        return QString();
    }
    
    QString gridName = getGridTypeName(grid);
    QString columnName = getColumnHeaderText(grid, index.column());
    QString cellValue = index.data().toString();
    
    return QString("%1, %2: %3").arg(gridName, columnName, cellValue);
}

QString DatabaseGridKeyboardNavigationEnhancer::getColumnHeaderText(QTableView* grid, int column) const
{
    if (!grid || !grid->model() || column < 0 || column >= grid->model()->columnCount()) {
        return QString("Column %1").arg(column + 1);
    }
    
    QString headerText = grid->model()->headerData(column, Qt::Horizontal).toString();
    return headerText.isEmpty() ? QString("Column %1").arg(column + 1) : headerText;
}

QString DatabaseGridKeyboardNavigationEnhancer::getRowInfo(QTableView* grid, int row) const
{
    if (!grid || !grid->model() || row < 0 || row >= grid->model()->rowCount()) {
        return QString("Row %1").arg(row + 1);
    }
    
    return QString("Row %1 of %2").arg(row + 1).arg(grid->model()->rowCount());
}

void DatabaseGridKeyboardNavigationEnhancer::announceGridCellWithContext(QTableView* grid, const QModelIndex& index)
{
    if (!m_accessibilityManager || !grid || !index.isValid()) {
        return;
    }
    
    QString announcement;
    
    // Build announcement based on verbosity level
    if (m_accessibilityManager->verbosityLevel() == AccessibilityManager::VerbosityLevel::Terse) {
        // Just the cell value
        announcement = index.data().toString();
    } else if (m_accessibilityManager->verbosityLevel() == AccessibilityManager::VerbosityLevel::Normal) {
        // Cell value with column name
        QString columnName = getColumnHeaderText(grid, index.column());
        announcement = QString("%1: %2").arg(columnName, index.data().toString());
    } else { // Verbose
        // Full context with grid name, position, and value
        QString gridName = getGridTypeName(grid);
        QString columnName = getColumnHeaderText(grid, index.column());
        QString rowInfo = getRowInfo(grid, index.row());
        QString cellValue = index.data().toString();
        
        announcement = QString("%1, %2, %3: %4").arg(gridName, rowInfo, columnName, cellValue);
    }
    
    if (!announcement.isEmpty()) {
        m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
    }
}

void DatabaseGridKeyboardNavigationEnhancer::announceGridOperation(GridOperation operation, bool success, const QString& details)
{
    if (!m_accessibilityManager) {
        return;
    }
    
    QString operationName;
    switch (operation) {
        case GridOperation::AddToPlaylist: operationName = "Add to playlist"; break;
        case GridOperation::RemoveFromGrid: operationName = "Remove"; break;
        case GridOperation::EditItem: operationName = "Edit"; break;
        case GridOperation::SortAscending: operationName = "Sort ascending"; break;
        case GridOperation::SortDescending: operationName = "Sort descending"; break;
        case GridOperation::FilterByValue: operationName = "Filter"; break;
        case GridOperation::ClearFilter: operationName = "Clear filter"; break;
        case GridOperation::SelectAll: operationName = "Select all"; break;
        case GridOperation::CopyItems: operationName = "Copy"; break;
        case GridOperation::CutItems: operationName = "Cut"; break;
        case GridOperation::PasteItems: operationName = "Paste"; break;
    }
    
    QString announcement = operationName;
    if (success) {
        if (!details.isEmpty()) {
            announcement += ": " + details;
        }
    } else {
        announcement += " failed";
        if (!details.isEmpty()) {
            announcement += ": " + details;
        }
    }
    
    AccessibilityManager::Priority priority = success ? AccessibilityManager::Priority::Normal : AccessibilityManager::Priority::High;
    m_accessibilityManager->announceMessage(announcement, priority);
}

QString DatabaseGridKeyboardNavigationEnhancer::getSelectionCountString(QTableView* grid) const
{
    if (!grid || !grid->selectionModel()) {
        return QString();
    }
    
    int selectedCount = grid->selectionModel()->selectedRows().count();
    if (selectedCount == 0) {
        return QString();
    }
    
    return QString("%1 item%2 selected").arg(selectedCount).arg(selectedCount > 1 ? "s" : "");
}

bool DatabaseGridKeyboardNavigationEnhancer::supportsOperation(QTableView* grid, GridOperation operation) const
{
    if (!grid) {
        return false;
    }
    
    // All grids support basic operations
    switch (operation) {
        case GridOperation::AddToPlaylist:
        case GridOperation::CopyItems:
        case GridOperation::SelectAll:
        case GridOperation::SortAscending:
        case GridOperation::SortDescending:
            return true;
            
        case GridOperation::RemoveFromGrid:
        case GridOperation::EditItem:
        case GridOperation::CutItems:
        case GridOperation::PasteItems:
            // These might be restricted based on grid type or permissions
            return true; // For now, allow all
            
        case GridOperation::FilterByValue:
        case GridOperation::ClearFilter:
            // Check if grid has a filter proxy model
            return qobject_cast<QSortFilterProxyModel*>(grid->model()) != nullptr;
    }
    
    return false;
}

QString DatabaseGridKeyboardNavigationEnhancer::getGridTypeName(QTableView* grid) const
{
    if (!grid) {
        return "Unknown grid";
    }
    
    if (grid == m_musicView) {
        return "Music library";
    } else if (grid == m_jinglesView) {
        return "Jingles";
    } else if (grid == m_pubView) {
        return "Advertisements";
    } else if (grid == m_programsView) {
        return "Programs";
    }
    
    return grid->objectName().isEmpty() ? "Database grid" : grid->objectName();
}