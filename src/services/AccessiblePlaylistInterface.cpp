#include "AccessiblePlaylistInterface.h"
#include "AccessibilityManager.h"
#include <QApplication>
#include <QListWidgetItem>
#include <QKeyEvent>
#include <QDebug>

// AccessiblePlaylistInterface implementation

AccessiblePlaylistInterface::AccessiblePlaylistInterface(QListWidget* listWidget, AccessibilityManager* manager)
    : m_listWidget(listWidget)
    , m_accessibilityManager(manager)
{
    Q_ASSERT(m_listWidget);
}

AccessiblePlaylistInterface::~AccessiblePlaylistInterface()
{
    // Clean up item interfaces
    qDeleteAll(m_itemInterfaces);
    m_itemInterfaces.clear();
}

bool AccessiblePlaylistInterface::isValid() const
{
    return m_listWidget && !m_listWidget->isHidden();
}

QObject* AccessiblePlaylistInterface::object() const
{
    return m_listWidget;
}

QWindow* AccessiblePlaylistInterface::window() const
{
    return m_listWidget ? m_listWidget->window()->windowHandle() : nullptr;
}

QAccessibleInterface* AccessiblePlaylistInterface::parent() const
{
    if (!m_listWidget || !m_listWidget->parentWidget()) {
        return nullptr;
    }
    return QAccessible::queryAccessibleInterface(m_listWidget->parentWidget());
}

QAccessibleInterface* AccessiblePlaylistInterface::child(int index) const
{
    if (!m_listWidget || index < 0 || index >= childCount()) {
        return nullptr;
    }

    if (!m_itemInterfaces.contains(index)) {
        m_itemInterfaces[index] = new AccessiblePlaylistItemInterface(
            m_listWidget, index, const_cast<AccessiblePlaylistInterface*>(this));
    }

    return m_itemInterfaces[index];
}

int AccessiblePlaylistInterface::childCount() const
{
    return m_listWidget ? m_listWidget->count() : 0;
}

int AccessiblePlaylistInterface::indexOfChild(const QAccessibleInterface* child) const
{
    if (!child) {
        return -1;
    }

    const AccessiblePlaylistItemInterface* itemInterface = 
        qobject_cast<const AccessiblePlaylistItemInterface*>(child);
    
    if (!itemInterface) {
        return -1;
    }

    return itemInterface->row();
}

QString AccessiblePlaylistInterface::text(QAccessible::Text t) const
{
    if (!m_listWidget) {
        return QString();
    }

    switch (t) {
    case QAccessible::Name:
        return m_listWidget->accessibleName().isEmpty() ? 
               QString("Playlist") : m_listWidget->accessibleName();
    case QAccessible::Description:
        return m_listWidget->accessibleDescription().isEmpty() ?
               QString("Playlist with %1 items. Use arrow keys to navigate, Ctrl+X to cut, Ctrl+V to paste, Delete to remove items")
               .arg(childCount()) : m_listWidget->accessibleDescription();
    case QAccessible::Help:
        return QString("Use arrow keys to navigate, Ctrl+Up/Down to move items, Ctrl+X to cut, Ctrl+V to paste, Delete to remove");
    case QAccessible::Value:
        return getSelectionInfo();
    default:
        return QString();
    }
}

void AccessiblePlaylistInterface::setText(QAccessible::Text t, const QString& text)
{
    if (!m_listWidget) {
        return;
    }

    switch (t) {
    case QAccessible::Name:
        m_listWidget->setAccessibleName(text);
        break;
    case QAccessible::Description:
        m_listWidget->setAccessibleDescription(text);
        break;
    default:
        break;
    }
}

QRect AccessiblePlaylistInterface::rect() const
{
    if (!m_listWidget) {
        return QRect();
    }
    return QRect(m_listWidget->mapToGlobal(QPoint(0, 0)), m_listWidget->size());
}

QAccessible::Role AccessiblePlaylistInterface::role() const
{
    return QAccessible::List;
}

QAccessible::State AccessiblePlaylistInterface::state() const
{
    QAccessible::State state;
    
    if (!m_listWidget) {
        state.invalid = true;
        return state;
    }

    if (!m_listWidget->isVisible()) {
        state.invisible = true;
    }
    
    if (!m_listWidget->isEnabled()) {
        state.disabled = true;
    }
    
    if (m_listWidget->hasFocus()) {
        state.focused = true;
    }
    
    if (m_listWidget->currentItem()) {
        state.selected = true;
    }

    state.focusable = true;
    state.selectable = true;

    return state;
}

QColor AccessiblePlaylistInterface::foregroundColor() const
{
    return m_listWidget ? m_listWidget->palette().color(QPalette::Text) : QColor();
}

QColor AccessiblePlaylistInterface::backgroundColor() const
{
    return m_listWidget ? m_listWidget->palette().color(QPalette::Base) : QColor();
}

QAccessibleInterface* AccessiblePlaylistInterface::focusChild() const
{
    if (!m_listWidget) {
        return nullptr;
    }

    const int currentRow = m_listWidget->currentRow();
    if (currentRow >= 0 && currentRow < childCount()) {
        return child(currentRow);
    }

    return nullptr;
}

QAccessibleInterface* AccessiblePlaylistInterface::childAt(int x, int y) const
{
    if (!m_listWidget) {
        return nullptr;
    }

    const QPoint localPos = m_listWidget->mapFromGlobal(QPoint(x, y));
    QListWidgetItem* item = m_listWidget->itemAt(localPos);
    
    if (!item) {
        return nullptr;
    }

    const int row = m_listWidget->row(item);
    return child(row);
}

void AccessiblePlaylistInterface::announceItemContext(int row)
{
    if (!m_accessibilityManager || !m_listWidget || row < 0 || row >= childCount()) {
        return;
    }

    const QString content = getItemContentWithContext(row);
    m_accessibilityManager->announceMessage(content);
}

void AccessiblePlaylistInterface::announcePlaylistModification(const QString& operation, const QString& itemText, const QString& position)
{
    if (!m_accessibilityManager) {
        return;
    }

    QString announcement = QString("%1: %2").arg(operation).arg(itemText);
    if (!position.isEmpty()) {
        announcement += QString(" %1").arg(position);
    }

    m_accessibilityManager->announceMessage(announcement);
}

QString AccessiblePlaylistInterface::getItemContentWithContext(int row) const
{
    if (!m_listWidget || row < 0 || row >= childCount()) {
        return QString();
    }

    QListWidgetItem* item = m_listWidget->item(row);
    if (!item) {
        return QString();
    }

    const QString itemText = item->text();
    const QString position = QString("Item %1 of %2").arg(row + 1).arg(childCount());
    
    QString context = QString("%1, %2").arg(itemText.isEmpty() ? "empty item" : itemText).arg(position);
    
    // Add selection state if applicable
    if (item->isSelected()) {
        context += " (selected)";
    }

    return context;
}

QString AccessiblePlaylistInterface::getSelectionInfo() const
{
    if (!m_listWidget) {
        return QString();
    }

    const int selectedCount = m_listWidget->selectedItems().count();
    const int totalCount = childCount();

    if (selectedCount == 0) {
        return QString("No items selected, %1 total items").arg(totalCount);
    } else if (selectedCount == 1) {
        const int currentRow = m_listWidget->currentRow();
        return QString("Item %1 of %2 selected").arg(currentRow + 1).arg(totalCount);
    } else {
        return QString("%1 items selected, %2 total items").arg(selectedCount).arg(totalCount);
    }
}

// AccessiblePlaylistItemInterface implementation

AccessiblePlaylistItemInterface::AccessiblePlaylistItemInterface(QListWidget* listWidget, int row, AccessiblePlaylistInterface* parent)
    : m_listWidget(listWidget)
    , m_parentInterface(parent)
    , m_row(row)
{
    Q_ASSERT(m_listWidget);
    Q_ASSERT(m_parentInterface);
}

AccessiblePlaylistItemInterface::~AccessiblePlaylistItemInterface()
{
}

bool AccessiblePlaylistItemInterface::isValid() const
{
    return m_listWidget && m_row >= 0 && m_row < m_listWidget->count();
}

QObject* AccessiblePlaylistItemInterface::object() const
{
    return m_listWidget;
}

QWindow* AccessiblePlaylistItemInterface::window() const
{
    return m_listWidget ? m_listWidget->window()->windowHandle() : nullptr;
}

QAccessibleInterface* AccessiblePlaylistItemInterface::parent() const
{
    return m_parentInterface;
}

QAccessibleInterface* AccessiblePlaylistItemInterface::child(int index) const
{
    Q_UNUSED(index)
    return nullptr; // Items don't have children
}

int AccessiblePlaylistItemInterface::childCount() const
{
    return 0; // Items don't have children
}

int AccessiblePlaylistItemInterface::indexOfChild(const QAccessibleInterface* child) const
{
    Q_UNUSED(child)
    return -1; // Items don't have children
}

QString AccessiblePlaylistItemInterface::text(QAccessible::Text t) const
{
    if (!isValid()) {
        return QString();
    }

    QListWidgetItem* item = m_listWidget->item(m_row);
    if (!item) {
        return QString();
    }
    
    switch (t) {
    case QAccessible::Name:
        return m_parentInterface->getItemContentWithContext(m_row);
    case QAccessible::Value:
        return item->text();
    case QAccessible::Description:
        return QString("Playlist item at position %1").arg(m_row + 1);
    default:
        return QString();
    }
}

void AccessiblePlaylistItemInterface::setText(QAccessible::Text t, const QString& text)
{
    if (!isValid() || t != QAccessible::Value) {
        return;
    }

    QListWidgetItem* item = m_listWidget->item(m_row);
    if (item) {
        item->setText(text);
    }
}

QRect AccessiblePlaylistItemInterface::rect() const
{
    if (!isValid()) {
        return QRect();
    }

    const QRect itemRect = m_listWidget->visualItemRect(m_listWidget->item(m_row));
    return QRect(m_listWidget->mapToGlobal(itemRect.topLeft()), itemRect.size());
}

QAccessible::Role AccessiblePlaylistItemInterface::role() const
{
    return QAccessible::ListItem;
}

QAccessible::State AccessiblePlaylistItemInterface::state() const
{
    QAccessible::State state;
    
    if (!isValid()) {
        state.invalid = true;
        return state;
    }

    if (!m_listWidget->isVisible()) {
        state.invisible = true;
    }
    
    if (!m_listWidget->isEnabled()) {
        state.disabled = true;
    }

    QListWidgetItem* item = m_listWidget->item(m_row);
    if (item) {
        if (item->isSelected()) {
            state.selected = true;
        }
        
        if (m_listWidget->currentRow() == m_row) {
            state.focused = true;
        }
    }

    state.focusable = true;
    state.selectable = true;

    return state;
}

QColor AccessiblePlaylistItemInterface::foregroundColor() const
{
    if (!isValid()) {
        return QColor();
    }

    QListWidgetItem* item = m_listWidget->item(m_row);
    if (item) {
        const QVariant foreground = item->data(Qt::ForegroundRole);
        if (foreground.canConvert<QColor>()) {
            return foreground.value<QColor>();
        }
    }
    
    return m_listWidget->palette().color(QPalette::Text);
}

QColor AccessiblePlaylistItemInterface::backgroundColor() const
{
    if (!isValid()) {
        return QColor();
    }

    QListWidgetItem* item = m_listWidget->item(m_row);
    if (item) {
        const QVariant background = item->data(Qt::BackgroundRole);
        if (background.canConvert<QColor>()) {
            return background.value<QColor>();
        }
    }
    
    return m_listWidget->palette().color(QPalette::Base);
}

QAccessibleInterface* AccessiblePlaylistItemInterface::focusChild() const
{
    return nullptr; // Items don't have children
}

QAccessibleInterface* AccessiblePlaylistItemInterface::childAt(int x, int y) const
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    return nullptr; // Items don't have children
}

// AccessiblePlaylistEnhancer implementation

AccessiblePlaylistEnhancer::AccessiblePlaylistEnhancer(QListWidget* listWidget, AccessibilityManager* manager, QObject* parent)
    : QObject(parent)
    , m_listWidget(listWidget)
    , m_accessibilityManager(manager)
    , m_playlistInterface(nullptr)
    , m_cutItem(nullptr)
    , m_cutItemRow(-1)
    , m_announcementTimer(new QTimer(this))
{
    Q_ASSERT(m_listWidget);
    Q_ASSERT(m_accessibilityManager);
    
    // Setup announcement timer
    m_announcementTimer->setSingleShot(true);
    m_announcementTimer->setInterval(ANNOUNCEMENT_DELAY_MS);
    connect(m_announcementTimer, &QTimer::timeout,
            this, &AccessiblePlaylistEnhancer::processDelayedAnnouncement);
}

AccessiblePlaylistEnhancer::~AccessiblePlaylistEnhancer()
{
    shutdown();
}

bool AccessiblePlaylistEnhancer::initialize()
{
    if (!m_listWidget || !m_accessibilityManager) {
        qWarning() << "Cannot initialize AccessiblePlaylistEnhancer: missing dependencies";
        return false;
    }
    
    // Create playlist interface
    m_playlistInterface = new AccessiblePlaylistInterface(m_listWidget, m_accessibilityManager);
    
    // Install event filter for keyboard handling
    m_listWidget->installEventFilter(this);
    
    // Connect to list signals
    connectListSignals();
    
    qDebug() << "AccessiblePlaylistEnhancer initialized for list widget";
    return true;
}

void AccessiblePlaylistEnhancer::shutdown()
{
    if (m_listWidget) {
        m_listWidget->removeEventFilter(this);
        disconnectListSignals();
    }
    
    delete m_playlistInterface;
    m_playlistInterface = nullptr;
    
    m_cutItem = nullptr;
    m_cutItemRow = -1;
    
    qDebug() << "AccessiblePlaylistEnhancer shutdown complete";
}

bool AccessiblePlaylistEnhancer::moveItemUp()
{
    if (!m_listWidget) {
        return false;
    }

    const int currentRow = m_listWidget->currentRow();
    if (currentRow <= 0) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Item is already at the top");
        }
        return false;
    }

    QListWidgetItem* item = m_listWidget->takeItem(currentRow);
    if (!item) {
        return false;
    }

    const int newRow = currentRow - 1;
    m_listWidget->insertItem(newRow, item);
    m_listWidget->setCurrentRow(newRow);

    if (m_playlistInterface) {
        m_playlistInterface->announcePlaylistModification(
            "Moved up", item->text(), QString("to position %1").arg(newRow + 1));
    }

    return true;
}

bool AccessiblePlaylistEnhancer::moveItemDown()
{
    if (!m_listWidget) {
        return false;
    }

    const int currentRow = m_listWidget->currentRow();
    if (currentRow < 0 || currentRow >= m_listWidget->count() - 1) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Item is already at the bottom");
        }
        return false;
    }

    QListWidgetItem* item = m_listWidget->takeItem(currentRow);
    if (!item) {
        return false;
    }

    const int newRow = currentRow + 1;
    m_listWidget->insertItem(newRow, item);
    m_listWidget->setCurrentRow(newRow);

    if (m_playlistInterface) {
        m_playlistInterface->announcePlaylistModification(
            "Moved down", item->text(), QString("to position %1").arg(newRow + 1));
    }

    return true;
}

bool AccessiblePlaylistEnhancer::cutItem()
{
    if (!m_listWidget) {
        return false;
    }

    const int currentRow = m_listWidget->currentRow();
    if (currentRow < 0) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("No item selected to cut");
        }
        return false;
    }

    // Clear previous cut item
    m_cutItem = nullptr;
    m_cutItemRow = -1;

    QListWidgetItem* item = m_listWidget->item(currentRow);
    if (!item) {
        return false;
    }

    // Store cut item information
    m_cutItem = item;
    m_cutItemRow = currentRow;

    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            QString("Cut item: %1").arg(item->text()));
    }

    return true;
}

bool AccessiblePlaylistEnhancer::pasteItem()
{
    if (!m_listWidget || !m_cutItem) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("No item to paste");
        }
        return false;
    }

    const int currentRow = m_listWidget->currentRow();
    const int insertRow = (currentRow >= 0) ? currentRow + 1 : m_listWidget->count();

    // Remove item from original position
    QListWidgetItem* item = m_listWidget->takeItem(m_cutItemRow);
    if (!item) {
        m_cutItem = nullptr;
        m_cutItemRow = -1;
        return false;
    }

    // Insert at new position
    const int actualInsertRow = (m_cutItemRow < insertRow) ? insertRow - 1 : insertRow;
    m_listWidget->insertItem(actualInsertRow, item);
    m_listWidget->setCurrentRow(actualInsertRow);

    if (m_playlistInterface) {
        m_playlistInterface->announcePlaylistModification(
            "Pasted", item->text(), QString("at position %1").arg(actualInsertRow + 1));
    }

    // Clear cut item
    m_cutItem = nullptr;
    m_cutItemRow = -1;

    return true;
}

bool AccessiblePlaylistEnhancer::deleteItem()
{
    if (!m_listWidget) {
        return false;
    }

    const int currentRow = m_listWidget->currentRow();
    if (currentRow < 0) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("No item selected to delete");
        }
        return false;
    }

    QListWidgetItem* item = m_listWidget->item(currentRow);
    if (!item) {
        return false;
    }

    const QString itemText = item->text();
    delete m_listWidget->takeItem(currentRow);

    // Clear cut item if it was the deleted item
    if (m_cutItem == item) {
        m_cutItem = nullptr;
        m_cutItemRow = -1;
    }

    if (m_playlistInterface) {
        m_playlistInterface->announcePlaylistModification("Removed", itemText);
    }

    return true;
}

bool AccessiblePlaylistEnhancer::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_listWidget) {
        return false;
    }
    
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        return handlePlaylistKeyboard(keyEvent);
    }
    
    return false;
}

void AccessiblePlaylistEnhancer::onCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous)
    
    if (current) {
        announceItemWithDelay(current);
    }
}

void AccessiblePlaylistEnhancer::onItemSelectionChanged()
{
    // Handle selection changes if needed
    // Currently handled by onCurrentItemChanged
}

void AccessiblePlaylistEnhancer::onModelChanged()
{
    // Clear cached interfaces when model changes
    if (m_playlistInterface) {
        // The interface will handle its own cleanup
    }
}

void AccessiblePlaylistEnhancer::processDelayedAnnouncement()
{
    if (!m_pendingAnnouncement.isEmpty() && m_accessibilityManager) {
        m_accessibilityManager->announceMessage(m_pendingAnnouncement);
        m_pendingAnnouncement.clear();
    }
}

bool AccessiblePlaylistEnhancer::handlePlaylistKeyboard(QKeyEvent* event)
{
    if (!event) {
        return false;
    }
    
    const Qt::KeyboardModifiers modifiers = event->modifiers();
    
    switch (event->key()) {
    case Qt::Key_Up:
        if (modifiers & Qt::ControlModifier) {
            // Ctrl+Up to move item up
            moveItemUp();
            return true;
        }
        break;
        
    case Qt::Key_Down:
        if (modifiers & Qt::ControlModifier) {
            // Ctrl+Down to move item down
            moveItemDown();
            return true;
        }
        break;
        
    case Qt::Key_X:
        if (modifiers & Qt::ControlModifier) {
            // Ctrl+X to cut item
            cutItem();
            return true;
        }
        break;
        
    case Qt::Key_V:
        if (modifiers & Qt::ControlModifier) {
            // Ctrl+V to paste item
            pasteItem();
            return true;
        }
        break;
        
    case Qt::Key_Delete:
        // Delete key to remove item
        deleteItem();
        return true;
        
    default:
        break;
    }
    
    return false;
}

void AccessiblePlaylistEnhancer::announceItemWithDelay(QListWidgetItem* item)
{
    if (!item || !m_listWidget) {
        return;
    }

    const int row = m_listWidget->row(item);
    if (row >= 0 && m_playlistInterface) {
        m_pendingAnnouncement = m_playlistInterface->getItemContentWithContext(row);
        m_announcementTimer->start();
    }
}

QString AccessiblePlaylistEnhancer::getItemPositionDescription(int row) const
{
    if (!m_listWidget) {
        return QString();
    }

    return QString("position %1 of %2").arg(row + 1).arg(m_listWidget->count());
}

void AccessiblePlaylistEnhancer::connectListSignals()
{
    if (!m_listWidget) {
        return;
    }
    
    connect(m_listWidget, &QListWidget::currentItemChanged,
            this, &AccessiblePlaylistEnhancer::onCurrentItemChanged);
    
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &AccessiblePlaylistEnhancer::onItemSelectionChanged);
    
    // Connect to model signals for change detection
    if (m_listWidget->model()) {
        connect(m_listWidget->model(), &QAbstractItemModel::modelReset,
                this, &AccessiblePlaylistEnhancer::onModelChanged);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsInserted,
                this, &AccessiblePlaylistEnhancer::onModelChanged);
        connect(m_listWidget->model(), &QAbstractItemModel::rowsRemoved,
                this, &AccessiblePlaylistEnhancer::onModelChanged);
    }
    
    qDebug() << "Connected to list widget signals for playlist enhancement";
}

void AccessiblePlaylistEnhancer::disconnectListSignals()
{
    if (!m_listWidget) {
        return;
    }
    
    disconnect(m_listWidget, nullptr, this, nullptr);
    
    if (m_listWidget->model()) {
        disconnect(m_listWidget->model(), nullptr, this, nullptr);
    }
    
    qDebug() << "Disconnected from list widget signals";
}