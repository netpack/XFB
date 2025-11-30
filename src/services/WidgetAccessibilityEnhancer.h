#ifndef WIDGETACCESSIBILITYENHANCER_H
#define WIDGETACCESSIBILITYENHANCER_H

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QSet>
#include <QHash>
#include <QAccessible>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableView>
#include <QListWidget>
#include <QTreeView>
#include <QTabWidget>
#include <QGroupBox>
#include <QProgressBar>

class AccessibilityManager;
class AccessibleTableEditingEnhancer;
class AccessiblePlaylistEnhancer;

/**
 * @brief Enhances existing Qt widgets with proper accessibility attributes
 * 
 * This class automatically discovers widgets and enhances them with accessibility
 * features such as proper labeling, role assignment, and state management.
 * It uses a QApplication-level event filter for automatic widget detection.
 * 
 * The enhancer provides:
 * - Automatic accessible name and description assignment
 * - Role detection and assignment for common Qt controls
 * - State management for interactive widgets
 * - Special handling for complex widgets like tables and lists
 * 
 * @since XFB 2.0
 */
class WidgetAccessibilityEnhancer : public QObject
{
    Q_OBJECT

public:
    explicit WidgetAccessibilityEnhancer(AccessibilityManager* manager, QObject* parent = nullptr);
    ~WidgetAccessibilityEnhancer() override;

    /**
     * @brief Initialize the widget enhancer
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the widget enhancer
     */
    void shutdown();

    /**
     * @brief Manually enhance a specific widget
     * @param widget The widget to enhance
     */
    void enhanceWidget(QWidget* widget);

    /**
     * @brief Set accessible name for a widget
     * @param widget The widget to set name for
     * @param name The accessible name
     */
    void setAccessibleName(QWidget* widget, const QString& name);

    /**
     * @brief Set accessible description for a widget
     * @param widget The widget to set description for
     * @param description The accessible description
     */
    void setAccessibleDescription(QWidget* widget, const QString& description);

    /**
     * @brief Set accessible role for a widget
     * @param widget The widget to set role for
     * @param role The accessible role
     */
    void setAccessibleRole(QWidget* widget, QAccessible::Role role);

    /**
     * @brief Enhance table view accessibility
     * @param tableView The table view to enhance
     */
    void enhanceTableView(QTableView* tableView);

    /**
     * @brief Enhance playlist (list widget) accessibility
     * @param playlist The list widget to enhance
     */
    void enhancePlaylist(QListWidget* playlist);

protected:
    /**
     * @brief Event filter for automatic widget detection
     * @param obj The object that received the event
     * @param event The event
     * @return true if the event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief Handle widget destruction
     * @param obj The destroyed widget
     */
    void onWidgetDestroyed(QObject* obj);

private:
    /**
     * @brief Automatically enhance a widget based on its type
     * @param widget The widget to auto-enhance
     */
    void autoEnhanceWidget(QWidget* widget);

    /**
     * @brief Generate accessible name for a widget
     * @param widget The widget to generate name for
     * @return Generated accessible name
     */
    QString generateAccessibleName(QWidget* widget);

    /**
     * @brief Generate accessible description for a widget
     * @param widget The widget to generate description for
     * @return Generated accessible description
     */
    QString generateAccessibleDescription(QWidget* widget);

    /**
     * @brief Determine appropriate role for a widget
     * @param widget The widget to determine role for
     * @return Appropriate accessible role
     */
    QAccessible::Role determineAccessibleRole(QWidget* widget);

    /**
     * @brief Setup grid accessibility for table views
     * @param table The table view to setup
     */
    void setupGridAccessibility(QTableView* table);

    /**
     * @brief Setup playlist accessibility for list widgets
     * @param list The list widget to setup
     */
    void setupPlaylistAccessibility(QListWidget* list);

    /**
     * @brief Install custom accessible interface for a widget
     * @param widget The widget to install interface for
     */
    void installAccessibleInterface(QWidget* widget);

    /**
     * @brief Find associated label for a widget
     * @param widget The widget to find label for
     * @return Associated label text, or empty string if none found
     */
    QString findAssociatedLabel(QWidget* widget);

    /**
     * @brief Check if widget should be enhanced
     * @param widget The widget to check
     * @return true if widget should be enhanced
     */
    bool shouldEnhanceWidget(QWidget* widget);

    /**
     * @brief Update widget accessibility metadata
     * @param widget The widget to update
     */
    void updateWidgetMetadata(QWidget* widget);

    // Member variables
    AccessibilityManager* m_accessibilityManager;
    QSet<QWidget*> m_enhancedWidgets;
    QHash<QWidget*, QString> m_widgetNames;
    QHash<QWidget*, QString> m_widgetDescriptions;
    // Temporarily disabled for beta build:
    // QHash<QTableView*, class AccessibleTableEditingEnhancer*> m_tableEditingEnhancers;
    // QHash<QListWidget*, class AccessiblePlaylistEnhancer*> m_playlistEnhancers;
    QSet<QTableView*> m_enhancedTableViews;
    QSet<QListWidget*> m_enhancedListWidgets;
    bool m_initialized;
    bool m_eventFilterInstalled;
};

#endif // WIDGETACCESSIBILITYENHANCER_H