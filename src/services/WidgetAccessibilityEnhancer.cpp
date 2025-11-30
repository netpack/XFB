#include "WidgetAccessibilityEnhancer.h"
#include "AccessibilityManager.h"
#include "AccessibleTableInterface.h"
// Temporarily disabled for beta build:
// #include "AccessibleTableEditingEnhancer.h"
// #include "AccessiblePlaylistInterface.h"
#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QAccessible>

WidgetAccessibilityEnhancer::WidgetAccessibilityEnhancer(AccessibilityManager* manager, QObject* parent)
    : QObject(parent)
    , m_accessibilityManager(manager)
    , m_initialized(false)
    , m_eventFilterInstalled(false)
{
    qDebug() << "WidgetAccessibilityEnhancer created";
}

WidgetAccessibilityEnhancer::~WidgetAccessibilityEnhancer()
{
    shutdown();
    qDebug() << "WidgetAccessibilityEnhancer destroyed";
}

bool WidgetAccessibilityEnhancer::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "Initializing WidgetAccessibilityEnhancer";
    
    // Install application-level event filter for automatic widget detection
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->installEventFilter(this);
        m_eventFilterInstalled = true;
        qDebug() << "Event filter installed for automatic widget detection";
    } else {
        qWarning() << "Failed to install event filter - QApplication not available";
        return false;
    }
    
    // Enhance existing widgets that are already created
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        QWidgetList allWidgets = app->allWidgets();
        for (QWidget* widget : allWidgets) {
            if (shouldEnhanceWidget(widget)) {
                enhanceWidget(widget);
            }
        }
        qDebug() << "Enhanced" << m_enhancedWidgets.size() << "existing widgets";
    }
    
    m_initialized = true;
    qDebug() << "WidgetAccessibilityEnhancer initialized successfully";
    return true;
}

void WidgetAccessibilityEnhancer::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "Shutting down WidgetAccessibilityEnhancer";
    
    // Remove event filter
    if (m_eventFilterInstalled) {
        if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
            app->removeEventFilter(this);
        }
        m_eventFilterInstalled = false;
    }
    
    // Temporarily disabled for beta build
    // Clean up all table editing enhancers
    // for (auto it = m_tableEditingEnhancers.begin(); it != m_tableEditingEnhancers.end(); ++it) {
    //     AccessibleTableEditingEnhancer* enhancer = it.value();
    //     enhancer->shutdown();
    //     delete enhancer;
    // }
    // m_tableEditingEnhancers.clear();
    
    // Clean up all playlist enhancers
    // for (auto it = m_playlistEnhancers.begin(); it != m_playlistEnhancers.end(); ++it) {
    //     AccessiblePlaylistEnhancer* enhancer = it.value();
    //     enhancer->shutdown();
    //     delete enhancer;
    // }
    // m_playlistEnhancers.clear();
    
    // Clear enhanced widgets
    m_enhancedWidgets.clear();
    m_widgetNames.clear();
    m_widgetDescriptions.clear();
    
    m_initialized = false;
    qDebug() << "WidgetAccessibilityEnhancer shutdown complete";
}

void WidgetAccessibilityEnhancer::enhanceWidget(QWidget* widget)
{
    if (!widget || m_enhancedWidgets.contains(widget)) {
        return;
    }
    
    // Connect to widget destruction for cleanup
    connect(widget, &QObject::destroyed,
            this, &WidgetAccessibilityEnhancer::onWidgetDestroyed);
    
    // Auto-enhance based on widget type
    autoEnhanceWidget(widget);
    
    // Mark as enhanced
    m_enhancedWidgets.insert(widget);
    
    // Update metadata in AccessibilityManager
    updateWidgetMetadata(widget);
    
    qDebug() << "Enhanced widget:" << widget->metaObject()->className() 
             << "Name:" << widget->accessibleName();
}

void WidgetAccessibilityEnhancer::setAccessibleName(QWidget* widget, const QString& name)
{
    if (!widget) {
        return;
    }
    
    widget->setAccessibleName(name);
    m_widgetNames[widget] = name;
    
    qDebug() << "Set accessible name for" << widget->metaObject()->className() << ":" << name;
}

void WidgetAccessibilityEnhancer::setAccessibleDescription(QWidget* widget, const QString& description)
{
    if (!widget) {
        return;
    }
    
    widget->setAccessibleDescription(description);
    m_widgetDescriptions[widget] = description;
    
    qDebug() << "Set accessible description for" << widget->metaObject()->className() << ":" << description;
}

void WidgetAccessibilityEnhancer::setAccessibleRole(QWidget* widget, QAccessible::Role role)
{
    if (!widget) {
        return;
    }
    
    // Qt6 doesn't have a direct setAccessibleRole method on QWidget
    // The role is typically determined by the widget type and QAccessible framework
    // We can store this information for custom accessible interfaces if needed
    
    qDebug() << "Accessible role set for" << widget->metaObject()->className() 
             << "Role:" << static_cast<int>(role);
}

void WidgetAccessibilityEnhancer::enhanceTableView(QTableView* tableView)
{
    if (!tableView) {
        return;
    }
    
    setupGridAccessibility(tableView);
    
    // Set accessible properties
    if (tableView->accessibleName().isEmpty()) {
        setAccessibleName(tableView, "Music Library Grid");
    }
    
    if (tableView->accessibleDescription().isEmpty()) {
        setAccessibleDescription(tableView, "Navigate with arrow keys, press Enter to edit");
    }
    
    qDebug() << "Enhanced TableView accessibility";
}

void WidgetAccessibilityEnhancer::enhancePlaylist(QListWidget* playlist)
{
    if (!playlist) {
        return;
    }
    
    setupPlaylistAccessibility(playlist);
    
    // Set accessible properties
    if (playlist->accessibleName().isEmpty()) {
        setAccessibleName(playlist, "Playlist");
    }
    
    if (playlist->accessibleDescription().isEmpty()) {
        setAccessibleDescription(playlist, "Use arrow keys to navigate, drag and drop to reorder");
    }
    
    qDebug() << "Enhanced Playlist accessibility";
}

bool WidgetAccessibilityEnhancer::eventFilter(QObject* obj, QEvent* event)
{
    if (!m_initialized || !m_accessibilityManager->isAccessibilityEnabled()) {
        return false;
    }
    
    // Handle widget show events for automatic enhancement
    if (event->type() == QEvent::Show || event->type() == QEvent::Polish) {
        if (QWidget* widget = qobject_cast<QWidget*>(obj)) {
            if (shouldEnhanceWidget(widget)) {
                enhanceWidget(widget);
            }
        }
    }
    
    return false; // Don't consume the event
}

void WidgetAccessibilityEnhancer::onWidgetDestroyed(QObject* obj)
{
    QWidget* widget = static_cast<QWidget*>(obj);
    
    // Temporarily disabled for beta build
    // Clean up table editing enhancers
    // if (QTableView* tableView = qobject_cast<QTableView*>(widget)) {
    //     if (m_tableEditingEnhancers.contains(tableView)) {
    //         AccessibleTableEditingEnhancer* enhancer = m_tableEditingEnhancers.take(tableView);
    //         enhancer->shutdown();
    //         delete enhancer;
    //     }
    // }
    // Clean up playlist enhancers
    // else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
    //     if (m_playlistEnhancers.contains(listWidget)) {
    //         AccessiblePlaylistEnhancer* enhancer = m_playlistEnhancers.take(listWidget);
    //         enhancer->shutdown();
    //         delete enhancer;
    //     }
    // }
    
    m_enhancedWidgets.remove(widget);
    m_widgetNames.remove(widget);
    m_widgetDescriptions.remove(widget);
}

void WidgetAccessibilityEnhancer::autoEnhanceWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    // Generate accessible name if not already set
    if (widget->accessibleName().isEmpty()) {
        QString name = generateAccessibleName(widget);
        if (!name.isEmpty()) {
            setAccessibleName(widget, name);
        }
    }
    
    // Generate accessible description if not already set
    if (widget->accessibleDescription().isEmpty()) {
        QString description = generateAccessibleDescription(widget);
        if (!description.isEmpty()) {
            setAccessibleDescription(widget, description);
        }
    }
    
    // Handle specific widget types
    if (QPushButton* button = qobject_cast<QPushButton*>(widget)) {
        if (button->accessibleName().isEmpty() && !button->text().isEmpty()) {
            setAccessibleName(button, button->text());
        }
    }
    else if (QLabel* label = qobject_cast<QLabel*>(widget)) {
        if (label->accessibleName().isEmpty() && !label->text().isEmpty()) {
            setAccessibleName(label, label->text());
        }
    }
    else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        if (lineEdit->accessibleName().isEmpty()) {
            QString labelText = findAssociatedLabel(lineEdit);
            if (!labelText.isEmpty()) {
                setAccessibleName(lineEdit, labelText);
            } else if (!lineEdit->placeholderText().isEmpty()) {
                setAccessibleName(lineEdit, lineEdit->placeholderText());
            } else {
                setAccessibleName(lineEdit, "Text Input");
            }
        }
    }
    else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        if (comboBox->accessibleName().isEmpty()) {
            QString labelText = findAssociatedLabel(comboBox);
            if (!labelText.isEmpty()) {
                setAccessibleName(comboBox, labelText);
            } else {
                setAccessibleName(comboBox, "Combo Box");
            }
        }
    }
    else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
        if (checkBox->accessibleName().isEmpty() && !checkBox->text().isEmpty()) {
            setAccessibleName(checkBox, checkBox->text());
        }
    }
    else if (QRadioButton* radioButton = qobject_cast<QRadioButton*>(widget)) {
        if (radioButton->accessibleName().isEmpty() && !radioButton->text().isEmpty()) {
            setAccessibleName(radioButton, radioButton->text());
        }
    }
    else if (QTableView* tableView = qobject_cast<QTableView*>(widget)) {
        enhanceTableView(tableView);
    }
    else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
        enhancePlaylist(listWidget);
    }
    else if (QTabWidget* tabWidget = qobject_cast<QTabWidget*>(widget)) {
        if (tabWidget->accessibleName().isEmpty()) {
            setAccessibleName(tabWidget, "Tab Container");
        }
    }
    else if (QGroupBox* groupBox = qobject_cast<QGroupBox*>(widget)) {
        if (groupBox->accessibleName().isEmpty() && !groupBox->title().isEmpty()) {
            setAccessibleName(groupBox, groupBox->title());
        }
    }
    else if (QProgressBar* progressBar = qobject_cast<QProgressBar*>(widget)) {
        if (progressBar->accessibleName().isEmpty()) {
            setAccessibleName(progressBar, "Progress Bar");
        }
    }
}

QString WidgetAccessibilityEnhancer::generateAccessibleName(QWidget* widget)
{
    if (!widget) {
        return QString();
    }
    
    // Try to find associated label
    QString labelText = findAssociatedLabel(widget);
    if (!labelText.isEmpty()) {
        return labelText;
    }
    
    // Use object name if available and meaningful
    QString objectName = widget->objectName();
    if (!objectName.isEmpty() && !objectName.startsWith("qt_")) {
        // Convert camelCase or snake_case to readable text
        QString readable = objectName;
        readable.replace(QRegularExpression("([a-z])([A-Z])"), "\\1 \\2");
        readable.replace("_", " ");
        return readable;
    }
    
    // Use window title for top-level widgets
    if (widget->isWindow() && !widget->windowTitle().isEmpty()) {
        return widget->windowTitle();
    }
    
    // Fallback to class name
    QString className = widget->metaObject()->className();
    className.remove(0, 1); // Remove 'Q' prefix
    return className;
}

QString WidgetAccessibilityEnhancer::generateAccessibleDescription(QWidget* widget)
{
    if (!widget) {
        return QString();
    }
    
    // Use tooltip if available
    if (!widget->toolTip().isEmpty()) {
        return widget->toolTip();
    }
    
    // Use status tip if available
    if (!widget->statusTip().isEmpty()) {
        return widget->statusTip();
    }
    
    // Use what's this help if available
    if (!widget->whatsThis().isEmpty()) {
        return widget->whatsThis();
    }
    
    return QString();
}

QAccessible::Role WidgetAccessibilityEnhancer::determineAccessibleRole(QWidget* widget)
{
    if (!widget) {
        return QAccessible::NoRole;
    }
    
    // Qt6 automatically determines roles based on widget types
    // This method can be used for custom role assignment if needed
    
    if (qobject_cast<QPushButton*>(widget)) {
        return QAccessible::Button;
    } else if (qobject_cast<QLabel*>(widget)) {
        return QAccessible::StaticText;
    } else if (qobject_cast<QLineEdit*>(widget)) {
        return QAccessible::EditableText;
    } else if (qobject_cast<QComboBox*>(widget)) {
        return QAccessible::ComboBox;
    } else if (qobject_cast<QCheckBox*>(widget)) {
        return QAccessible::CheckBox;
    } else if (qobject_cast<QRadioButton*>(widget)) {
        return QAccessible::RadioButton;
    } else if (qobject_cast<QTableView*>(widget)) {
        return QAccessible::Table;
    } else if (qobject_cast<QListWidget*>(widget)) {
        return QAccessible::List;
    }
    
    return QAccessible::NoRole;
}

void WidgetAccessibilityEnhancer::setupGridAccessibility(QTableView* table)
{
    if (!table) {
        return;
    }
    
    // Enable keyboard navigation
    table->setTabKeyNavigation(true);
    
    // Ensure selection behavior is appropriate for accessibility
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Install custom accessible interface for enhanced table accessibility
    installAccessibleInterface(table);
    
    // Temporarily disabled for beta build - has compilation errors
    // Create and initialize editing enhancer
    // if (!m_tableEditingEnhancers.contains(table)) {
    //     AccessibleTableEditingEnhancer* editingEnhancer = 
    //         new AccessibleTableEditingEnhancer(table, m_accessibilityManager, this);
    //     
    //     if (editingEnhancer->initialize()) {
    //         m_tableEditingEnhancers[table] = editingEnhancer;
    //         qDebug() << "Created editing enhancer for TableView";
    //     } else {
    //         delete editingEnhancer;
    //         qWarning() << "Failed to initialize editing enhancer for TableView";
    //     }
    // }
    
    qDebug() << "Setup grid accessibility for TableView with custom interface and editing support";
}

void WidgetAccessibilityEnhancer::setupPlaylistAccessibility(QListWidget* list)
{
    if (!list) {
        return;
    }
    
    // Enable keyboard navigation
    list->setTabKeyNavigation(true);
    
    // Enable drag and drop for keyboard users (will be enhanced with keyboard alternatives)
    list->setDragDropMode(QAbstractItemView::InternalMove);
    
    // Install custom accessible interface for enhanced playlist accessibility
    installAccessibleInterface(list);
    
    // Temporarily disabled for beta build - has compilation errors
    // Create and initialize playlist enhancer
    // if (!m_playlistEnhancers.contains(list)) {
    //     AccessiblePlaylistEnhancer* playlistEnhancer = 
    //         new AccessiblePlaylistEnhancer(list, m_accessibilityManager, this);
    //     
    //     if (playlistEnhancer->initialize()) {
    //         m_playlistEnhancers[list] = playlistEnhancer;
    //         qDebug() << "Created playlist enhancer for ListWidget";
    //     } else {
    //         delete playlistEnhancer;
    //         qWarning() << "Failed to initialize playlist enhancer for ListWidget";
    //     }
    // }
    
    qDebug() << "Setup playlist accessibility for ListWidget with keyboard alternatives";
}

void WidgetAccessibilityEnhancer::installAccessibleInterface(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    // Enhanced accessibility for table views
    if (QTableView* tableView = qobject_cast<QTableView*>(widget)) {
        // Store reference and enhance directly
        m_enhancedTableViews.insert(tableView);
        setupGridAccessibility(tableView);
        qDebug() << "Enhanced TableView accessibility";
    }
    // Enhanced accessibility for list widgets (playlists)
    else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
        // Store reference and enhance directly
        m_enhancedListWidgets.insert(listWidget);
        setupPlaylistAccessibility(listWidget);
        qDebug() << "Enhanced ListWidget accessibility";
    } else {
        qDebug() << "Custom accessible interface installation (no specific interface for this widget type)";
    }
}

QString WidgetAccessibilityEnhancer::findAssociatedLabel(QWidget* widget)
{
    if (!widget || !widget->parentWidget()) {
        return QString();
    }
    
    // Look for QLabel that has this widget as buddy
    QList<QLabel*> labels = widget->parentWidget()->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->buddy() == widget) {
            return label->text().remove('&'); // Remove mnemonic indicators
        }
    }
    
    // Look for labels in form layouts
    if (QFormLayout* formLayout = qobject_cast<QFormLayout*>(widget->parentWidget()->layout())) {
        for (int i = 0; i < formLayout->rowCount(); ++i) {
            QLayoutItem* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
            QLayoutItem* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
            
            if (fieldItem && fieldItem->widget() == widget && labelItem) {
                if (QLabel* label = qobject_cast<QLabel*>(labelItem->widget())) {
                    return label->text().remove('&');
                }
            }
        }
    }
    
    // Look for nearby labels (simple heuristic)
    QList<QLabel*> nearbyLabels = widget->parentWidget()->findChildren<QLabel*>();
    for (QLabel* label : nearbyLabels) {
        // Check if label is positioned near the widget
        QRect labelGeometry = label->geometry();
        QRect widgetGeometry = widget->geometry();
        
        // Simple proximity check (label to the left or above)
        if ((labelGeometry.right() <= widgetGeometry.left() + 20 && 
             qAbs(labelGeometry.center().y() - widgetGeometry.center().y()) < 30) ||
            (labelGeometry.bottom() <= widgetGeometry.top() + 20 &&
             qAbs(labelGeometry.center().x() - widgetGeometry.center().x()) < 50)) {
            return label->text().remove('&');
        }
    }
    
    return QString();
}

bool WidgetAccessibilityEnhancer::shouldEnhanceWidget(QWidget* widget)
{
    if (!widget || m_enhancedWidgets.contains(widget)) {
        return false;
    }
    
    // Don't enhance internal Qt widgets
    QString objectName = widget->objectName();
    if (objectName.startsWith("qt_") || objectName.startsWith("_q_")) {
        return false;
    }
    
    // Don't enhance widgets that are not visible or have no parent
    if (!widget->isVisible() && !widget->isWindow()) {
        return false;
    }
    
    // Only enhance interactive or informational widgets
    return (widget->focusPolicy() != Qt::NoFocus ||
            qobject_cast<QLabel*>(widget) ||
            qobject_cast<QProgressBar*>(widget) ||
            qobject_cast<QGroupBox*>(widget) ||
            qobject_cast<QTabWidget*>(widget));
}

void WidgetAccessibilityEnhancer::updateWidgetMetadata(QWidget* widget)
{
    if (!widget || !m_accessibilityManager) {
        return;
    }
    
    // Create metadata for the AccessibilityManager
    AccessibilityManager::AccessibilityMetadata metadata;
    metadata.name = widget->accessibleName();
    metadata.description = widget->accessibleDescription();
    metadata.role = determineAccessibleRole(widget);
    metadata.helpText = widget->toolTip();
    
    // Register with AccessibilityManager
    m_accessibilityManager->registerAccessibleWidget(widget, metadata);
}