#include "AccessibleTableEditingEnhancer.h"
#include "AccessibilityManager.h"
#include "AccessibleTableInterface.h"
#include <QApplication>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDebug>

AccessibleTableEditingEnhancer::AccessibleTableEditingEnhancer(QTableView* tableView, AccessibilityManager* manager, QObject* parent)
    : QObject(parent)
    , m_tableView(tableView)
    , m_accessibilityManager(manager)
    , m_tableInterface(nullptr)
    , m_isEditing(false)
    , m_editingEnabled(true)
    , m_announcementTimer(new QTimer(this))
{
    Q_ASSERT(m_tableView);
    Q_ASSERT(m_accessibilityManager);
    
    // Setup announcement timer
    m_announcementTimer->setSingleShot(true);
    m_announcementTimer->setInterval(ANNOUNCEMENT_DELAY_MS);
    connect(m_announcementTimer, &QTimer::timeout,
            this, &AccessibleTableEditingEnhancer::processDelayedAnnouncement);
}

AccessibleTableEditingEnhancer::~AccessibleTableEditingEnhancer()
{
    shutdown();
}

bool AccessibleTableEditingEnhancer::initialize()
{
    if (!m_tableView || !m_accessibilityManager) {
        qWarning() << "Cannot initialize AccessibleTableEditingEnhancer: missing dependencies";
        return false;
    }
    
    // Install event filter for keyboard handling
    m_tableView->installEventFilter(this);
    
    // Connect to table signals
    connectTableSignals();
    
    // Setup keyboard shortcuts
    setupKeyboardShortcuts();
    
    qDebug() << "AccessibleTableEditingEnhancer initialized for table view";
    return true;
}

void AccessibleTableEditingEnhancer::shutdown()
{
    if (m_tableView) {
        m_tableView->removeEventFilter(this);
        disconnectTableSignals();
    }
    
    m_previousValues.clear();
    m_currentEditIndex = QModelIndex();
    m_isEditing = false;
    
    qDebug() << "AccessibleTableEditingEnhancer shutdown complete";
}

bool AccessibleTableEditingEnhancer::isCellBeingEdited(int row, int column) const
{
    if (!m_isEditing || !m_currentEditIndex.isValid()) {
        return false;
    }
    
    return m_currentEditIndex.row() == row && m_currentEditIndex.column() == column;
}

bool AccessibleTableEditingEnhancer::enterEditMode()
{
    if (!m_tableView || !m_tableView->selectionModel()) {
        return false;
    }
    
    const QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    if (!currentIndex.isValid() || !isCellEditable(currentIndex)) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Cell is not editable");
        }
        return false;
    }
    
    // Store original value for change detection
    m_originalValue = getCellValue(currentIndex);
    m_currentEditIndex = currentIndex;
    m_isEditing = true;
    
    // Trigger edit mode in the table view
    m_tableView->edit(currentIndex);
    
    // Announce edit state change
    announceEditStateChange(true, currentIndex);
    
    return true;
}

bool AccessibleTableEditingEnhancer::exitEditMode()
{
    if (!m_isEditing || !m_currentEditIndex.isValid()) {
        return false;
    }
    
    // Commit the edit
    if (m_tableView->state() == QAbstractItemView::EditingState) {
        // Close the editor and commit data
        QWidget* editor = m_tableView->indexWidget(m_currentEditIndex);
        if (editor) {
            m_tableView->commitData(editor);
            m_tableView->closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
        }
    }
    
    // Get new value for confirmation
    const QString newValue = getCellValue(m_currentEditIndex);
    
    // Announce confirmation
    announceEditConfirmation(m_currentEditIndex, m_originalValue, newValue);
    
    // Reset edit state
    m_isEditing = false;
    const QModelIndex editedIndex = m_currentEditIndex;
    m_currentEditIndex = QModelIndex();
    m_originalValue.clear();
    
    // Announce edit state change
    announceEditStateChange(false, editedIndex);
    
    return true;
}

bool AccessibleTableEditingEnhancer::cancelEditMode()
{
    if (!m_isEditing || !m_currentEditIndex.isValid()) {
        return false;
    }
    
    // Cancel the edit
    if (m_tableView->state() == QAbstractItemView::EditingState) {
        QWidget* editor = m_tableView->indexWidget(m_currentEditIndex);
        if (editor) {
            m_tableView->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
        }
    }
    
    // Announce cancellation
    if (m_accessibilityManager) {
        const QString columnName = getColumnName(m_currentEditIndex.column());
        const QString announcement = QString("Edit cancelled for %1").arg(columnName);
        m_accessibilityManager->announceMessage(announcement);
    }
    
    // Reset edit state
    m_isEditing = false;
    const QModelIndex editedIndex = m_currentEditIndex;
    m_currentEditIndex = QModelIndex();
    m_originalValue.clear();
    
    // Announce edit state change
    announceEditStateChange(false, editedIndex);
    
    return true;
}

bool AccessibleTableEditingEnhancer::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_tableView || !m_editingEnabled) {
        return false;
    }
    
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        return handleEditingKeyboard(keyEvent);
    }
    
    return false;
}

void AccessibleTableEditingEnhancer::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous)
    
    // If we were editing and selection changed, exit edit mode
    if (m_isEditing && m_currentEditIndex.isValid() && current != m_currentEditIndex) {
        exitEditMode();
    }
    
    // Announce current cell information
    if (current.isValid() && m_accessibilityManager) {
        const QString cellValue = getCellValue(current);
        const QString columnName = getColumnName(current.column());
        
        QString announcement;
        if (cellValue.isEmpty()) {
            announcement = QString("%1: empty").arg(columnName);
        } else {
            announcement = QString("%1: %2").arg(columnName).arg(cellValue);
        }
        
        // Add editability information
        if (isCellEditable(current)) {
            announcement += " (editable)";
        }
        
        m_pendingAnnouncement = announcement;
        m_announcementTimer->start();
    }
}

void AccessibleTableEditingEnhancer::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(roles)
    
    // Check if the currently edited cell was changed
    if (m_isEditing && m_currentEditIndex.isValid()) {
        if (m_currentEditIndex.row() >= topLeft.row() && m_currentEditIndex.row() <= bottomRight.row() &&
            m_currentEditIndex.column() >= topLeft.column() && m_currentEditIndex.column() <= bottomRight.column()) {
            
            // The edited cell was updated, we can now exit edit mode
            const QString newValue = getCellValue(m_currentEditIndex);
            announceEditConfirmation(m_currentEditIndex, m_originalValue, newValue);
            
            m_isEditing = false;
            m_currentEditIndex = QModelIndex();
            m_originalValue.clear();
        }
    }
}

void AccessibleTableEditingEnhancer::onEditTriggered(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    
    m_currentEditIndex = index;
    m_originalValue = getCellValue(index);
    m_isEditing = true;
    
    announceEditStateChange(true, index);
}

void AccessibleTableEditingEnhancer::processDelayedAnnouncement()
{
    if (!m_pendingAnnouncement.isEmpty() && m_accessibilityManager) {
        m_accessibilityManager->announceMessage(m_pendingAnnouncement);
        m_pendingAnnouncement.clear();
    }
}

bool AccessibleTableEditingEnhancer::handleEditingKeyboard(QKeyEvent* event)
{
    if (!event || !m_tableView->selectionModel()) {
        return false;
    }
    
    const QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    
    switch (event->key()) {
    case Qt::Key_F2:
        // F2 to enter edit mode
        if (!m_isEditing && currentIndex.isValid()) {
            enterEditMode();
            return true;
        }
        break;
        
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_isEditing) {
            // Enter to confirm edit
            exitEditMode();
            return true;
        } else if (currentIndex.isValid()) {
            // Enter to start editing
            enterEditMode();
            return true;
        }
        break;
        
    case Qt::Key_Escape:
        if (m_isEditing) {
            // Escape to cancel edit
            cancelEditMode();
            return true;
        }
        break;
        
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        if (m_isEditing) {
            // Tab to move to next cell and continue editing
            exitEditMode();
            // Let the table handle the tab navigation
            return false;
        }
        break;
        
    default:
        // For other keys, if we're not editing and the cell is editable,
        // start editing and pass the key to the editor
        if (!m_isEditing && currentIndex.isValid() && isCellEditable(currentIndex)) {
            if (event->text().length() == 1 && event->text().at(0).isPrint()) {
                enterEditMode();
                // The key will be processed by the editor
                return false;
            }
        }
        break;
    }
    
    return false;
}

void AccessibleTableEditingEnhancer::announceEditStateChange(bool entering, const QModelIndex& index)
{
    if (!m_accessibilityManager || !index.isValid()) {
        return;
    }
    
    const QString columnName = getColumnName(index.column());
    const QString cellValue = getCellValue(index);
    
    QString announcement;
    if (entering) {
        announcement = QString("Editing %1, current value: %2")
            .arg(columnName)
            .arg(cellValue.isEmpty() ? "empty" : cellValue);
    } else {
        announcement = "Edit mode exited";
    }
    
    m_accessibilityManager->announceMessage(announcement);
}

void AccessibleTableEditingEnhancer::announceEditConfirmation(const QModelIndex& index, const QString& oldValue, const QString& newValue)
{
    if (!m_accessibilityManager || !index.isValid()) {
        return;
    }
    
    const QString columnName = getColumnName(index.column());
    
    QString announcement;
    if (oldValue != newValue) {
        announcement = QString("%1 changed from %2 to %3")
            .arg(columnName)
            .arg(oldValue.isEmpty() ? "empty" : oldValue)
            .arg(newValue.isEmpty() ? "empty" : newValue);
    } else {
        announcement = QString("%1 unchanged").arg(columnName);
    }
    
    m_accessibilityManager->announceMessage(announcement);
}

QString AccessibleTableEditingEnhancer::getCellValue(const QModelIndex& index) const
{
    if (!index.isValid() || !m_tableView->model()) {
        return QString();
    }
    
    return m_tableView->model()->data(index).toString();
}

QString AccessibleTableEditingEnhancer::getColumnName(int column) const
{
    if (!m_tableView->model()) {
        return QString("Column %1").arg(column + 1);
    }
    
    const QVariant headerData = m_tableView->model()->headerData(column, Qt::Horizontal);
    const QString headerText = headerData.toString();
    
    if (headerText.isEmpty()) {
        return QString("Column %1").arg(column + 1);
    }
    
    return headerText;
}

bool AccessibleTableEditingEnhancer::isCellEditable(const QModelIndex& index) const
{
    if (!index.isValid() || !m_tableView->model()) {
        return false;
    }
    
    const Qt::ItemFlags flags = m_tableView->model()->flags(index);
    return flags & Qt::ItemIsEditable;
}

void AccessibleTableEditingEnhancer::setupKeyboardShortcuts()
{
    // Keyboard shortcuts are handled in the event filter
    // This method is for future expansion if needed
    qDebug() << "Keyboard shortcuts for table editing configured";
}

void AccessibleTableEditingEnhancer::connectTableSignals()
{
    if (!m_tableView) {
        return;
    }
    
    // Connect to selection model signals
    if (m_tableView->selectionModel()) {
        connect(m_tableView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &AccessibleTableEditingEnhancer::onSelectionChanged);
    }
    
    // Connect to model signals
    if (m_tableView->model()) {
        connect(m_tableView->model(), &QAbstractItemModel::dataChanged,
                this, &AccessibleTableEditingEnhancer::onDataChanged);
    }
    
    qDebug() << "Connected to table view signals for editing enhancement";
}

void AccessibleTableEditingEnhancer::disconnectTableSignals()
{
    if (!m_tableView) {
        return;
    }
    
    // Disconnect from selection model
    if (m_tableView->selectionModel()) {
        disconnect(m_tableView->selectionModel(), nullptr, this, nullptr);
    }
    
    // Disconnect from model
    if (m_tableView->model()) {
        disconnect(m_tableView->model(), nullptr, this, nullptr);
    }
    
    qDebug() << "Disconnected from table view signals";
}