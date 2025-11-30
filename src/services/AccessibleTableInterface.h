#ifndef ACCESSIBLETABLEINTERFACE_H
#define ACCESSIBLETABLEINTERFACE_H

#include <QAccessibleInterface>
#include <QAccessibleTableInterface>
#include <QAccessibleTableCellInterface>
#include <QTableView>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QHeaderView>

class AccessibilityManager;

/**
 * @brief Custom accessible interface for QTableView widgets
 * 
 * This class provides comprehensive accessibility support for table views,
 * including proper header announcements, cell navigation with context,
 * and state management for ORCA screen reader integration.
 * 
 * Features:
 * - Row and column header announcements
 * - Cell content with proper context information
 * - Navigation state tracking
 * - Edit mode state announcements
 * - Selection state management
 * 
 * @since XFB 2.0
 */
class AccessibleTableInterface : public QAccessibleInterface, public QAccessibleTableInterface
{
public:
    explicit AccessibleTableInterface(QTableView* tableView, AccessibilityManager* manager = nullptr);
    ~AccessibleTableInterface() override;

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

    // QAccessibleTableInterface implementation
    QAccessibleInterface* cellAt(int row, int column) const override;
    QAccessibleInterface* caption() const override;
    QAccessibleInterface* summary() const override;
    QString columnDescription(int column) const override;
    QString rowDescription(int row) const override;
    int columnCount() const override;
    int rowCount() const override;
    int selectedCellCount() const override;
    int selectedColumnCount() const override;
    int selectedRowCount() const override;
    QList<QAccessibleInterface*> selectedCells() const override;
    QList<int> selectedColumns() const override;
    QList<int> selectedRows() const override;
    bool isColumnSelected(int column) const override;
    bool isRowSelected(int row) const override;
    bool selectRow(int row) override;
    bool selectColumn(int column) override;
    bool unselectRow(int row) override;
    bool unselectColumn(int column) override;
    void modelChange(QAccessibleTableModelChangeEvent* event) override;

    /**
     * @brief Get the table view widget
     * @return Pointer to the table view
     */
    QTableView* tableView() const { return m_tableView; }

    /**
     * @brief Announce current cell context
     * @param row The row index
     * @param column The column index
     */
    void announceCellContext(int row, int column);

    /**
     * @brief Announce header information
     * @param orientation Header orientation (horizontal/vertical)
     * @param section Header section index
     */
    void announceHeader(Qt::Orientation orientation, int section);

    /**
     * @brief Update accessibility state for editing
     * @param editing true if entering edit mode, false if exiting
     * @param row The row being edited
     * @param column The column being edited
     */
    void updateEditingState(bool editing, int row = -1, int column = -1);

private:
    /**
     * @brief Get formatted cell content with context
     * @param row The row index
     * @param column The column index
     * @return Formatted cell content string
     */
    QString getCellContentWithContext(int row, int column) const;

    /**
     * @brief Get column header text
     * @param column The column index
     * @return Column header text
     */
    QString getColumnHeader(int column) const;

    /**
     * @brief Get row header text
     * @param row The row index
     * @return Row header text
     */
    QString getRowHeader(int row) const;

    /**
     * @brief Check if cell is currently selected
     * @param row The row index
     * @param column The column index
     * @return true if cell is selected
     */
    bool isCellSelected(int row, int column) const;

    /**
     * @brief Get current selection information
     * @return Formatted selection information string
     */
    QString getSelectionInfo() const;

    QTableView* m_tableView;
    AccessibilityManager* m_accessibilityManager;
    mutable QHash<QPair<int, int>, QAccessibleInterface*> m_cellInterfaces;
};

/**
 * @brief Accessible interface for individual table cells
 */
class AccessibleTableCellInterface : public QAccessibleInterface, public QAccessibleTableCellInterface
{
public:
    AccessibleTableCellInterface(QTableView* tableView, int row, int column, AccessibleTableInterface* parent);
    ~AccessibleTableCellInterface() override;

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

    // QAccessibleTableCellInterface implementation
    int columnExtent() const override;
    QList<QAccessibleInterface*> columnHeaderCells() const override;
    int columnIndex() const override;
    int rowExtent() const override;
    QList<QAccessibleInterface*> rowHeaderCells() const override;
    int rowIndex() const override;
    bool isSelected() const override;
    QAccessibleInterface* table() const override;

    /**
     * @brief Get the row index of this cell
     * @return Row index
     */
    int row() const { return m_row; }

    /**
     * @brief Get the column index of this cell
     * @return Column index
     */
    int column() const { return m_column; }

private:
    QTableView* m_tableView;
    AccessibleTableInterface* m_parentInterface;
    int m_row;
    int m_column;
};

#endif // ACCESSIBLETABLEINTERFACE_H