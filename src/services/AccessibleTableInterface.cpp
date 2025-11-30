#include "AccessibleTableInterface.h"
#include "AccessibilityManager.h"
#include <QApplication>
#include <QHeaderView>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QDebug>

AccessibleTableInterface::AccessibleTableInterface(QTableView* tableView, AccessibilityManager* manager)
    : m_tableView(tableView)
    , m_accessibilityManager(manager)
{
    Q_ASSERT(m_tableView);
}

AccessibleTableInterface::~AccessibleTableInterface()
{
    // Clean up cell interfaces
    qDeleteAll(m_cellInterfaces);
    m_cellInterfaces.clear();
}

bool AccessibleTableInterface::isValid() const
{
    return m_tableView && !m_tableView->isHidden();
}

QObject* AccessibleTableInterface::object() const
{
    return m_tableView;
}

QWindow* AccessibleTableInterface::window() const
{
    return m_tableView ? m_tableView->window()->windowHandle() : nullptr;
}

QAccessibleInterface* AccessibleTableInterface::parent() const
{
    if (!m_tableView || !m_tableView->parentWidget()) {
        return nullptr;
    }
    return QAccessible::queryAccessibleInterface(m_tableView->parentWidget());
}

QAccessibleInterface* AccessibleTableInterface::child(int index) const
{
    if (!m_tableView || !m_tableView->model()) {
        return nullptr;
    }

    const int rows = rowCount();
    const int cols = columnCount();
    const int totalCells = rows * cols;

    if (index < 0 || index >= totalCells) {
        return nullptr;
    }

    const int row = index / cols;
    const int column = index % cols;

    return cellAt(row, column);
}

int AccessibleTableInterface::childCount() const
{
    return rowCount() * columnCount();
}

int AccessibleTableInterface::indexOfChild(const QAccessibleInterface* child) const
{
    if (!child) {
        return -1;
    }

    const AccessibleTableCellInterface* cellInterface = 
        qobject_cast<const AccessibleTableCellInterface*>(child);
    
    if (!cellInterface) {
        return -1;
    }

    const int row = cellInterface->row();
    const int column = cellInterface->column();
    const int cols = columnCount();

    return row * cols + column;
}

QString AccessibleTableInterface::text(QAccessible::Text t) const
{
    if (!m_tableView) {
        return QString();
    }

    switch (t) {
    case QAccessible::Name:
        return m_tableView->accessibleName().isEmpty() ? 
               QString("Music Library Table") : m_tableView->accessibleName();
    case QAccessible::Description:
        return m_tableView->accessibleDescription().isEmpty() ?
               QString("Table containing music library with %1 rows and %2 columns")
               .arg(rowCount()).arg(columnCount()) : m_tableView->accessibleDescription();
    case QAccessible::Help:
        return QString("Use arrow keys to navigate, Enter to edit, Escape to cancel editing");
    case QAccessible::Value:
        return getSelectionInfo();
    default:
        return QString();
    }
}

void AccessibleTableInterface::setText(QAccessible::Text t, const QString& text)
{
    if (!m_tableView) {
        return;
    }

    switch (t) {
    case QAccessible::Name:
        m_tableView->setAccessibleName(text);
        break;
    case QAccessible::Description:
        m_tableView->setAccessibleDescription(text);
        break;
    default:
        break;
    }
}

QRect AccessibleTableInterface::rect() const
{
    if (!m_tableView) {
        return QRect();
    }
    return QRect(m_tableView->mapToGlobal(QPoint(0, 0)), m_tableView->size());
}

QAccessible::Role AccessibleTableInterface::role() const
{
    return QAccessible::Table;
}

QAccessible::State AccessibleTableInterface::state() const
{
    QAccessible::State state;
    
    if (!m_tableView) {
        state.invalid = true;
        return state;
    }

    if (!m_tableView->isVisible()) {
        state.invisible = true;
    }
    
    if (!m_tableView->isEnabled()) {
        state.disabled = true;
    }
    
    if (m_tableView->hasFocus()) {
        state.focused = true;
    }
    
    if (m_tableView->selectionModel() && m_tableView->selectionModel()->hasSelection()) {
        state.selected = true;
    }

    state.focusable = true;
    state.selectable = true;

    return state;
}

QColor AccessibleTableInterface::foregroundColor() const
{
    return m_tableView ? m_tableView->palette().color(QPalette::Text) : QColor();
}

QColor AccessibleTableInterface::backgroundColor() const
{
    return m_tableView ? m_tableView->palette().color(QPalette::Base) : QColor();
}

QAccessibleInterface* AccessibleTableInterface::focusChild() const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return nullptr;
    }

    const QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return nullptr;
    }

    return cellAt(currentIndex.row(), currentIndex.column());
}

QAccessibleInterface* AccessibleTableInterface::childAt(int x, int y) const
{
    if (!m_tableView) {
        return nullptr;
    }

    const QPoint localPos = m_tableView->mapFromGlobal(QPoint(x, y));
    const QModelIndex index = m_tableView->indexAt(localPos);
    
    if (!index.isValid()) {
        return nullptr;
    }

    return cellAt(index.row(), index.column());
}

// QAccessibleTableInterface implementation

QAccessibleInterface* AccessibleTableInterface::cellAt(int row, int column) const
{
    if (!m_tableView || !m_tableView->model()) {
        return nullptr;
    }

    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount()) {
        return nullptr;
    }

    const QPair<int, int> cellKey(row, column);
    
    if (!m_cellInterfaces.contains(cellKey)) {
        m_cellInterfaces[cellKey] = new AccessibleTableCellInterface(
            m_tableView, row, column, const_cast<AccessibleTableInterface*>(this));
    }

    return m_cellInterfaces[cellKey];
}

QAccessibleInterface* AccessibleTableInterface::caption() const
{
    // Tables in XFB don't typically have captions
    return nullptr;
}

QAccessibleInterface* AccessibleTableInterface::summary() const
{
    // Tables in XFB don't typically have summaries
    return nullptr;
}

QString AccessibleTableInterface::columnDescription(int column) const
{
    if (!m_tableView || !m_tableView->model()) {
        return QString();
    }

    const QString header = getColumnHeader(column);
    if (!header.isEmpty()) {
        return QString("Column %1: %2").arg(column + 1).arg(header);
    }
    
    return QString("Column %1").arg(column + 1);
}

QString AccessibleTableInterface::rowDescription(int row) const
{
    if (!m_tableView || !m_tableView->model()) {
        return QString();
    }

    const QString header = getRowHeader(row);
    if (!header.isEmpty()) {
        return QString("Row %1: %2").arg(row + 1).arg(header);
    }
    
    return QString("Row %1").arg(row + 1);
}

int AccessibleTableInterface::columnCount() const
{
    if (!m_tableView || !m_tableView->model()) {
        return 0;
    }
    return m_tableView->model()->columnCount();
}

int AccessibleTableInterface::rowCount() const
{
    if (!m_tableView || !m_tableView->model()) {
        return 0;
    }
    return m_tableView->model()->rowCount();
}

int AccessibleTableInterface::selectedCellCount() const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return 0;
    }
    return m_tableView->selectionModel()->selectedIndexes().count();
}

int AccessibleTableInterface::selectedColumnCount() const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return 0;
    }
    return m_tableView->selectionModel()->selectedColumns().count();
}

int AccessibleTableInterface::selectedRowCount() const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return 0;
    }
    return m_tableView->selectionModel()->selectedRows().count();
}

QList<QAccessibleInterface*> AccessibleTableInterface::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    
    if (!m_tableView || !m_tableView->selectionModel()) {
        return cells;
    }

    const QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedIndexes();
    for (const QModelIndex& index : selectedIndexes) {
        if (QAccessibleInterface* cell = cellAt(index.row(), index.column())) {
            cells.append(cell);
        }
    }

    return cells;
}

QList<int> AccessibleTableInterface::selectedColumns() const
{
    QList<int> columns;
    
    if (!m_tableView || !m_tableView->selectionModel()) {
        return columns;
    }

    const QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedColumns();
    for (const QModelIndex& index : selectedIndexes) {
        columns.append(index.column());
    }

    return columns;
}

QList<int> AccessibleTableInterface::selectedRows() const
{
    QList<int> rows;
    
    if (!m_tableView || !m_tableView->selectionModel()) {
        return rows;
    }

    const QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedRows();
    for (const QModelIndex& index : selectedIndexes) {
        rows.append(index.row());
    }

    return rows;
}

bool AccessibleTableInterface::isColumnSelected(int column) const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return false;
    }

    const QModelIndexList selectedColumns = m_tableView->selectionModel()->selectedColumns();
    for (const QModelIndex& index : selectedColumns) {
        if (index.column() == column) {
            return true;
        }
    }

    return false;
}

bool AccessibleTableInterface::isRowSelected(int row) const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return false;
    }

    const QModelIndexList selectedRows = m_tableView->selectionModel()->selectedRows();
    for (const QModelIndex& index : selectedRows) {
        if (index.row() == row) {
            return true;
        }
    }

    return false;
}

bool AccessibleTableInterface::selectRow(int row)
{
    if (!m_tableView || !m_tableView->selectionModel() || row < 0 || row >= rowCount()) {
        return false;
    }

    m_tableView->selectRow(row);
    
    // Announce the selection
    if (m_accessibilityManager) {
        const QString announcement = QString("Row %1 selected").arg(row + 1);
        m_accessibilityManager->announceMessage(announcement);
    }

    return true;
}

bool AccessibleTableInterface::selectColumn(int column)
{
    if (!m_tableView || !m_tableView->selectionModel() || column < 0 || column >= columnCount()) {
        return false;
    }

    m_tableView->selectColumn(column);
    
    // Announce the selection
    if (m_accessibilityManager) {
        const QString header = getColumnHeader(column);
        const QString announcement = header.isEmpty() ? 
            QString("Column %1 selected").arg(column + 1) :
            QString("Column %1 %2 selected").arg(column + 1).arg(header);
        m_accessibilityManager->announceMessage(announcement);
    }

    return true;
}

bool AccessibleTableInterface::unselectRow(int row)
{
    if (!m_tableView || !m_tableView->selectionModel() || row < 0 || row >= rowCount()) {
        return false;
    }

    const QModelIndex topLeft = m_tableView->model()->index(row, 0);
    const QModelIndex bottomRight = m_tableView->model()->index(row, columnCount() - 1);
    const QItemSelection selection(topLeft, bottomRight);
    
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::Deselect);
    
    // Announce the deselection
    if (m_accessibilityManager) {
        const QString announcement = QString("Row %1 deselected").arg(row + 1);
        m_accessibilityManager->announceMessage(announcement);
    }

    return true;
}

bool AccessibleTableInterface::unselectColumn(int column)
{
    if (!m_tableView || !m_tableView->selectionModel() || column < 0 || column >= columnCount()) {
        return false;
    }

    const QModelIndex topLeft = m_tableView->model()->index(0, column);
    const QModelIndex bottomRight = m_tableView->model()->index(rowCount() - 1, column);
    const QItemSelection selection(topLeft, bottomRight);
    
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::Deselect);
    
    // Announce the deselection
    if (m_accessibilityManager) {
        const QString header = getColumnHeader(column);
        const QString announcement = header.isEmpty() ? 
            QString("Column %1 deselected").arg(column + 1) :
            QString("Column %1 %2 deselected").arg(column + 1).arg(header);
        m_accessibilityManager->announceMessage(announcement);
    }

    return true;
}

void AccessibleTableInterface::modelChange(QAccessibleTableModelChangeEvent* event)
{
    if (!event) {
        return;
    }

    // Clear cached cell interfaces when model changes
    qDeleteAll(m_cellInterfaces);
    m_cellInterfaces.clear();

    // Announce model changes if accessibility manager is available
    if (m_accessibilityManager) {
        QString announcement;
        
        switch (event->modelChangeType()) {
        case QAccessibleTableModelChangeEvent::ModelReset:
            announcement = "Table data refreshed";
            break;
        case QAccessibleTableModelChangeEvent::DataChanged:
            announcement = "Table data updated";
            break;
        case QAccessibleTableModelChangeEvent::RowsInserted:
            announcement = QString("%1 rows added to table").arg(event->lastRow() - event->firstRow() + 1);
            break;
        case QAccessibleTableModelChangeEvent::RowsRemoved:
            announcement = QString("%1 rows removed from table").arg(event->lastRow() - event->firstRow() + 1);
            break;
        case QAccessibleTableModelChangeEvent::ColumnsInserted:
            announcement = QString("%1 columns added to table").arg(event->lastColumn() - event->firstColumn() + 1);
            break;
        case QAccessibleTableModelChangeEvent::ColumnsRemoved:
            announcement = QString("%1 columns removed from table").arg(event->lastColumn() - event->firstColumn() + 1);
            break;
        }
        
        if (!announcement.isEmpty()) {
            m_accessibilityManager->announceMessage(announcement);
        }
    }
}

void AccessibleTableInterface::announceCellContext(int row, int column)
{
    if (!m_accessibilityManager || !m_tableView || !m_tableView->model()) {
        return;
    }

    const QString content = getCellContentWithContext(row, column);
    m_accessibilityManager->announceMessage(content);
}

void AccessibleTableInterface::announceHeader(Qt::Orientation orientation, int section)
{
    if (!m_accessibilityManager || !m_tableView) {
        return;
    }

    QString announcement;
    
    if (orientation == Qt::Horizontal) {
        const QString header = getColumnHeader(section);
        announcement = header.isEmpty() ? 
            QString("Column %1").arg(section + 1) :
            QString("Column %1: %2").arg(section + 1).arg(header);
    } else {
        const QString header = getRowHeader(section);
        announcement = header.isEmpty() ? 
            QString("Row %1").arg(section + 1) :
            QString("Row %1: %2").arg(section + 1).arg(header);
    }

    m_accessibilityManager->announceMessage(announcement);
}

void AccessibleTableInterface::updateEditingState(bool editing, int row, int column)
{
    if (!m_accessibilityManager) {
        return;
    }

    QString announcement;
    
    if (editing) {
        if (row >= 0 && column >= 0) {
            const QString columnHeader = getColumnHeader(column);
            const QString cellValue = m_tableView->model()->data(
                m_tableView->model()->index(row, column)).toString();
            
            announcement = columnHeader.isEmpty() ?
                QString("Editing cell row %1 column %2, current value: %3")
                .arg(row + 1).arg(column + 1).arg(cellValue) :
                QString("Editing %1, current value: %2")
                .arg(columnHeader).arg(cellValue);
        } else {
            announcement = "Entering edit mode";
        }
    } else {
        announcement = "Exiting edit mode";
    }

    m_accessibilityManager->announceMessage(announcement);
}

// Private helper methods

QString AccessibleTableInterface::getCellContentWithContext(int row, int column) const
{
    if (!m_tableView || !m_tableView->model()) {
        return QString();
    }

    const QModelIndex index = m_tableView->model()->index(row, column);
    if (!index.isValid()) {
        return QString();
    }

    const QString cellValue = m_tableView->model()->data(index).toString();
    const QString columnHeader = getColumnHeader(column);
    
    QString context;
    if (!columnHeader.isEmpty()) {
        context = QString("%1: %2").arg(columnHeader).arg(cellValue.isEmpty() ? "empty" : cellValue);
    } else {
        context = QString("Row %1, Column %2: %3")
            .arg(row + 1).arg(column + 1).arg(cellValue.isEmpty() ? "empty" : cellValue);
    }

    // Add selection state if applicable
    if (isCellSelected(row, column)) {
        context += " (selected)";
    }

    return context;
}

QString AccessibleTableInterface::getColumnHeader(int column) const
{
    if (!m_tableView || !m_tableView->model()) {
        return QString();
    }

    const QVariant headerData = m_tableView->model()->headerData(column, Qt::Horizontal);
    return headerData.toString();
}

QString AccessibleTableInterface::getRowHeader(int row) const
{
    if (!m_tableView || !m_tableView->model()) {
        return QString();
    }

    const QVariant headerData = m_tableView->model()->headerData(row, Qt::Vertical);
    return headerData.toString();
}

bool AccessibleTableInterface::isCellSelected(int row, int column) const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return false;
    }

    const QModelIndex index = m_tableView->model()->index(row, column);
    return m_tableView->selectionModel()->isSelected(index);
}

QString AccessibleTableInterface::getSelectionInfo() const
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return QString();
    }

    const int selectedCells = selectedCellCount();
    const int selectedRows = selectedRowCount();
    const int selectedCols = selectedColumnCount();

    if (selectedCells == 0) {
        return "No selection";
    }

    if (selectedRows > 0 && selectedCols > 0) {
        return QString("%1 rows and %2 columns selected").arg(selectedRows).arg(selectedCols);
    } else if (selectedRows > 0) {
        return QString("%1 rows selected").arg(selectedRows);
    } else if (selectedCols > 0) {
        return QString("%1 columns selected").arg(selectedCols);
    } else {
        return QString("%1 cells selected").arg(selectedCells);
    }
}

// AccessibleTableCellInterface implementation

AccessibleTableCellInterface::AccessibleTableCellInterface(QTableView* tableView, int row, int column, AccessibleTableInterface* parent)
    : m_tableView(tableView)
    , m_parentInterface(parent)
    , m_row(row)
    , m_column(column)
{
    Q_ASSERT(m_tableView);
    Q_ASSERT(m_parentInterface);
}

AccessibleTableCellInterface::~AccessibleTableCellInterface()
{
}

bool AccessibleTableCellInterface::isValid() const
{
    return m_tableView && m_tableView->model() && 
           m_row >= 0 && m_row < m_tableView->model()->rowCount() &&
           m_column >= 0 && m_column < m_tableView->model()->columnCount();
}

QObject* AccessibleTableCellInterface::object() const
{
    return m_tableView;
}

QWindow* AccessibleTableCellInterface::window() const
{
    return m_tableView ? m_tableView->window()->windowHandle() : nullptr;
}

QAccessibleInterface* AccessibleTableCellInterface::parent() const
{
    return m_parentInterface;
}

QAccessibleInterface* AccessibleTableCellInterface::child(int index) const
{
    Q_UNUSED(index)
    return nullptr; // Cells don't have children
}

int AccessibleTableCellInterface::childCount() const
{
    return 0; // Cells don't have children
}

int AccessibleTableCellInterface::indexOfChild(const QAccessibleInterface* child) const
{
    Q_UNUSED(child)
    return -1; // Cells don't have children
}

QString AccessibleTableCellInterface::text(QAccessible::Text t) const
{
    if (!isValid()) {
        return QString();
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    
    switch (t) {
    case QAccessible::Name:
        return m_parentInterface->getCellContentWithContext(m_row, m_column);
    case QAccessible::Value:
        return m_tableView->model()->data(index).toString();
    case QAccessible::Description:
        return QString("Cell at row %1, column %2").arg(m_row + 1).arg(m_column + 1);
    default:
        return QString();
    }
}

void AccessibleTableCellInterface::setText(QAccessible::Text t, const QString& text)
{
    if (!isValid() || t != QAccessible::Value) {
        return;
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    m_tableView->model()->setData(index, text);
}

QRect AccessibleTableCellInterface::rect() const
{
    if (!isValid()) {
        return QRect();
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    const QRect cellRect = m_tableView->visualRect(index);
    return QRect(m_tableView->mapToGlobal(cellRect.topLeft()), cellRect.size());
}

QAccessible::Role AccessibleTableCellInterface::role() const
{
    return QAccessible::Cell;
}

QAccessible::State AccessibleTableCellInterface::state() const
{
    QAccessible::State state;
    
    if (!isValid()) {
        state.invalid = true;
        return state;
    }

    if (!m_tableView->isVisible()) {
        state.invisible = true;
    }
    
    if (!m_tableView->isEnabled()) {
        state.disabled = true;
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    
    if (m_tableView->selectionModel() && m_tableView->selectionModel()->isSelected(index)) {
        state.selected = true;
    }
    
    if (m_tableView->selectionModel() && m_tableView->selectionModel()->currentIndex() == index) {
        state.focused = true;
    }

    const Qt::ItemFlags flags = m_tableView->model()->flags(index);
    if (flags & Qt::ItemIsEditable) {
        state.editable = true;
    }

    state.focusable = true;
    state.selectable = true;

    return state;
}

QColor AccessibleTableCellInterface::foregroundColor() const
{
    if (!isValid()) {
        return QColor();
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    const QVariant foreground = m_tableView->model()->data(index, Qt::ForegroundRole);
    
    if (foreground.canConvert<QColor>()) {
        return foreground.value<QColor>();
    }
    
    return m_tableView->palette().color(QPalette::Text);
}

QColor AccessibleTableCellInterface::backgroundColor() const
{
    if (!isValid()) {
        return QColor();
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    const QVariant background = m_tableView->model()->data(index, Qt::BackgroundRole);
    
    if (background.canConvert<QColor>()) {
        return background.value<QColor>();
    }
    
    return m_tableView->palette().color(QPalette::Base);
}

QAccessibleInterface* AccessibleTableCellInterface::focusChild() const
{
    return nullptr; // Cells don't have children
}

QAccessibleInterface* AccessibleTableCellInterface::childAt(int x, int y) const
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    return nullptr; // Cells don't have children
}

// QAccessibleTableCellInterface implementation

int AccessibleTableCellInterface::columnExtent() const
{
    return 1; // Single cell
}

QList<QAccessibleInterface*> AccessibleTableCellInterface::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headers;
    // XFB tables don't typically have complex header structures
    return headers;
}

int AccessibleTableCellInterface::columnIndex() const
{
    return m_column;
}

int AccessibleTableCellInterface::rowExtent() const
{
    return 1; // Single cell
}

QList<QAccessibleInterface*> AccessibleTableCellInterface::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headers;
    // XFB tables don't typically have complex header structures
    return headers;
}

int AccessibleTableCellInterface::rowIndex() const
{
    return m_row;
}

bool AccessibleTableCellInterface::isSelected() const
{
    if (!isValid() || !m_tableView->selectionModel()) {
        return false;
    }

    const QModelIndex index = m_tableView->model()->index(m_row, m_column);
    return m_tableView->selectionModel()->isSelected(index);
}

QAccessibleInterface* AccessibleTableCellInterface::table() const
{
    return m_parentInterface;
}