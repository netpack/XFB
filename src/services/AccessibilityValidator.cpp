#include "AccessibilityValidator.h"
#include "AccessibilityManager.h"
#include <QApplication>
#include <QAccessible>
#include <QDebug>
#include <QDateTime>

AccessibilityValidator::AccessibilityValidator(AccessibilityManager* manager, QObject* parent)
    : QObject(parent)
    , m_accessibilityManager(manager)
    , m_validationTimer(new QTimer(this))
    , m_currentProgress(0)
{
    Q_ASSERT(m_accessibilityManager);
    
    m_validationTimer->setSingleShot(true);
    connect(m_validationTimer, &QTimer::timeout,
            this, &AccessibilityValidator::validateAllWidgets);
}

AccessibilityValidator::~AccessibilityValidator()
{
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::runFullValidation()
{
    QList<ValidationResult> results;
    
    emit validationProgress(0, "Starting accessibility validation");
    
    // Check if accessibility is enabled
    if (!isAccessibilityEnabled()) {
        results.append(ValidationResult("System", "Accessibility Status", false,
                                      "Accessibility is not enabled",
                                      "Enable accessibility features in XFB settings"));
        return results;
    }
    
    emit validationProgress(10, "Finding accessible widgets");
    
    // Find all widgets that should be accessible
    QList<QWidget*> widgets = findAccessibleWidgets();
    
    int totalWidgets = widgets.size();
    int currentWidget = 0;
    
    for (QWidget* widget : widgets) {
        currentWidget++;
        int progress = 10 + (currentWidget * 80 / totalWidgets);
        
        QString widgetType = getWidgetTypeDescription(widget);
        emit validationProgress(progress, QString("Validating %1").arg(widgetType));
        
        // Validate based on widget type
        if (QTableView* tableView = qobject_cast<QTableView*>(widget)) {
            results.append(validateTableView(tableView));
        } else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
            results.append(validatePlaylist(listWidget));
        } else {
            // General widget validation
            results.append(validateAccessibleInterface(widget));
            results.append(validateAccessibleProperties(widget));
            results.append(validateKeyboardNavigation(widget));
        }
    }
    
    emit validationProgress(100, "Validation complete");
    emit validationComplete(results);
    
    return results;
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::validateTableView(QTableView* tableView)
{
    QList<ValidationResult> results;
    
    if (!tableView) {
        results.append(ValidationResult("Table", "Null Check", false, "Table view is null"));
        return results;
    }
    
    QString tableName = tableView->objectName().isEmpty() ? "Table" : tableView->objectName();
    
    // Basic accessibility interface validation
    results.append(validateAccessibleInterface(tableView));
    results.append(validateAccessibleProperties(tableView));
    
    // Table-specific validations
    results.append(validateTableSpecificFeatures(tableView));
    
    // Keyboard navigation validation
    results.append(validateKeyboardNavigation(tableView));
    
    // Focus handling validation
    results.append(validateFocusHandling(tableView));
    
    return results;
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::validatePlaylist(QListWidget* listWidget)
{
    QList<ValidationResult> results;
    
    if (!listWidget) {
        results.append(ValidationResult("Playlist", "Null Check", false, "List widget is null"));
        return results;
    }
    
    QString playlistName = listWidget->objectName().isEmpty() ? "Playlist" : listWidget->objectName();
    
    // Basic accessibility interface validation
    results.append(validateAccessibleInterface(listWidget));
    results.append(validateAccessibleProperties(listWidget));
    
    // Playlist-specific validations
    results.append(validatePlaylistSpecificFeatures(listWidget));
    
    // Keyboard navigation validation
    results.append(validateKeyboardNavigation(listWidget));
    
    // Focus handling validation
    results.append(validateFocusHandling(listWidget));
    
    return results;
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::validateKeyboardNavigation(QWidget* widget)
{
    QList<ValidationResult> results;
    
    if (!widget) {
        results.append(ValidationResult("Navigation", "Null Check", false, "Widget is null"));
        return results;
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // Check if widget accepts focus
    if (!widget->focusPolicy() != Qt::NoFocus) {
        results.append(ValidationResult("Navigation", "Focus Policy", true,
                                      QString("%1 accepts keyboard focus").arg(widgetName)));
    } else {
        results.append(ValidationResult("Navigation", "Focus Policy", false,
                                      QString("%1 does not accept keyboard focus").arg(widgetName),
                                      "Set appropriate focus policy for keyboard navigation"));
    }
    
    // Check tab key navigation
    if (widget->property("tabKeyNavigation").toBool()) {
        results.append(ValidationResult("Navigation", "Tab Navigation", true,
                                      QString("%1 supports tab key navigation").arg(widgetName)));
    } else {
        results.append(ValidationResult("Navigation", "Tab Navigation", false,
                                      QString("%1 does not support tab key navigation").arg(widgetName),
                                      "Enable tab key navigation for accessibility"));
    }
    
    return results;
}

QString AccessibilityValidator::generateReport(const QList<ValidationResult>& results)
{
    QString report;
    QDateTime now = QDateTime::currentDateTime();
    
    report += "# XFB Accessibility Validation Report\n\n";
    report += QString("Generated: %1\n\n").arg(now.toString(Qt::ISODate));
    
    // Summary
    ValidationSummary summary = getSummary(results);
    report += "## Summary\n\n";
    report += QString("- Total Tests: %1\n").arg(summary.totalTests);
    report += QString("- Passed: %1\n").arg(summary.passedTests);
    report += QString("- Failed: %1\n").arg(summary.failedTests);
    report += QString("- Pass Rate: %1%\n").arg(summary.passRate, 0, 'f', 1);
    report += QString("- Critical Issues: %1\n\n").arg(summary.criticalIssues);
    
    // Group results by component
    QMap<QString, QList<ValidationResult>> groupedResults;
    for (const ValidationResult& result : results) {
        groupedResults[result.component].append(result);
    }
    
    // Generate detailed results
    report += "## Detailed Results\n\n";
    
    for (auto it = groupedResults.begin(); it != groupedResults.end(); ++it) {
        const QString& component = it.key();
        const QList<ValidationResult>& componentResults = it.value();
        
        report += QString("### %1\n\n").arg(component);
        
        for (const ValidationResult& result : componentResults) {
            QString status = result.passed ? "✓ PASS" : "✗ FAIL";
            report += QString("- **%1**: %2 - %3\n").arg(result.testName).arg(status).arg(result.message);
            
            if (!result.passed && !result.recommendation.isEmpty()) {
                report += QString("  - *Recommendation*: %1\n").arg(result.recommendation);
            }
        }
        report += "\n";
    }
    
    // Recommendations section
    QStringList recommendations;
    for (const ValidationResult& result : results) {
        if (!result.passed && !result.recommendation.isEmpty()) {
            recommendations.append(result.recommendation);
        }
    }
    
    if (!recommendations.isEmpty()) {
        report += "## Recommendations\n\n";
        recommendations.removeDuplicates();
        for (const QString& rec : recommendations) {
            report += QString("- %1\n").arg(rec);
        }
        report += "\n";
    }
    
    return report;
}

bool AccessibilityValidator::isAccessibilityEnabled() const
{
    return m_accessibilityManager && m_accessibilityManager->isAccessibilityEnabled();
}

AccessibilityValidator::ValidationSummary AccessibilityValidator::getSummary(const QList<ValidationResult>& results)
{
    ValidationSummary summary;
    summary.totalTests = results.size();
    summary.passedTests = 0;
    summary.failedTests = 0;
    summary.criticalIssues = 0;
    
    for (const ValidationResult& result : results) {
        if (result.passed) {
            summary.passedTests++;
        } else {
            summary.failedTests++;
            
            // Count critical issues (system-level failures)
            if (result.component == "System" || result.testName.contains("Interface")) {
                summary.criticalIssues++;
            }
        }
    }
    
    summary.passRate = summary.totalTests > 0 ? 
        (static_cast<double>(summary.passedTests) / summary.totalTests) * 100.0 : 0.0;
    
    return summary;
}

void AccessibilityValidator::validateAllWidgets()
{
    m_currentResults = runFullValidation();
}

AccessibilityValidator::ValidationResult AccessibilityValidator::validateAccessibleInterface(QWidget* widget)
{
    if (!widget) {
        return ValidationResult("Interface", "Widget Check", false, "Widget is null");
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // Check if widget has accessible interface
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    if (interface) {
        bool isValid = interface->isValid();
        delete interface;
        
        if (isValid) {
            return ValidationResult("Interface", "Accessible Interface", true,
                                  QString("%1 has valid accessible interface").arg(widgetName));
        } else {
            return ValidationResult("Interface", "Accessible Interface", false,
                                  QString("%1 has invalid accessible interface").arg(widgetName),
                                  "Fix accessible interface implementation");
        }
    } else {
        return ValidationResult("Interface", "Accessible Interface", false,
                              QString("%1 has no accessible interface").arg(widgetName),
                              "Implement accessible interface for this widget");
    }
}

AccessibilityValidator::ValidationResult AccessibilityValidator::validateAccessibleProperties(QWidget* widget)
{
    if (!widget) {
        return ValidationResult("Properties", "Widget Check", false, "Widget is null");
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // Check accessible name
    QString accessibleName = widget->accessibleName();
    if (accessibleName.isEmpty()) {
        return ValidationResult("Properties", "Accessible Name", false,
                              QString("%1 has no accessible name").arg(widgetName),
                              "Set meaningful accessible name for screen reader users");
    } else {
        return ValidationResult("Properties", "Accessible Name", true,
                              QString("%1 has accessible name: '%2'").arg(widgetName).arg(accessibleName));
    }
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::validateTableSpecificFeatures(QTableView* tableView)
{
    QList<ValidationResult> results;
    
    if (!tableView) {
        results.append(ValidationResult("Table", "Null Check", false, "Table view is null"));
        return results;
    }
    
    // Check if table has model
    if (tableView->model()) {
        results.append(ValidationResult("Table", "Data Model", true, "Table has data model"));
        
        // Check if model has data
        int rowCount = tableView->model()->rowCount();
        int columnCount = tableView->model()->columnCount();
        
        if (rowCount > 0 && columnCount > 0) {
            results.append(ValidationResult("Table", "Data Content", true,
                                          QString("Table has %1 rows and %2 columns").arg(rowCount).arg(columnCount)));
        } else {
            results.append(ValidationResult("Table", "Data Content", false,
                                          "Table has no data",
                                          "Load test data for accessibility validation"));
        }
        
        // Check column headers
        bool hasHeaders = false;
        for (int col = 0; col < columnCount; ++col) {
            QVariant headerData = tableView->model()->headerData(col, Qt::Horizontal);
            if (!headerData.toString().isEmpty()) {
                hasHeaders = true;
                break;
            }
        }
        
        if (hasHeaders) {
            results.append(ValidationResult("Table", "Column Headers", true, "Table has column headers"));
        } else {
            results.append(ValidationResult("Table", "Column Headers", false,
                                          "Table has no column headers",
                                          "Add descriptive column headers for screen reader users"));
        }
    } else {
        results.append(ValidationResult("Table", "Data Model", false,
                                      "Table has no data model",
                                      "Set data model for table view"));
    }
    
    // Check selection model
    if (tableView->selectionModel()) {
        results.append(ValidationResult("Table", "Selection Model", true, "Table has selection model"));
    } else {
        results.append(ValidationResult("Table", "Selection Model", false,
                                      "Table has no selection model",
                                      "Ensure table has proper selection model"));
    }
    
    return results;
}

QList<AccessibilityValidator::ValidationResult> AccessibilityValidator::validatePlaylistSpecificFeatures(QListWidget* listWidget)
{
    QList<ValidationResult> results;
    
    if (!listWidget) {
        results.append(ValidationResult("Playlist", "Null Check", false, "List widget is null"));
        return results;
    }
    
    // Check item count
    int itemCount = listWidget->count();
    if (itemCount > 0) {
        results.append(ValidationResult("Playlist", "Item Count", true,
                                      QString("Playlist has %1 items").arg(itemCount)));
        
        // Check if items have text
        bool hasItemText = false;
        for (int i = 0; i < qMin(itemCount, 5); ++i) { // Check first 5 items
            QListWidgetItem* item = listWidget->item(i);
            if (item && !item->text().isEmpty()) {
                hasItemText = true;
                break;
            }
        }
        
        if (hasItemText) {
            results.append(ValidationResult("Playlist", "Item Content", true, "Playlist items have text content"));
        } else {
            results.append(ValidationResult("Playlist", "Item Content", false,
                                          "Playlist items have no text content",
                                          "Ensure playlist items have meaningful text for screen readers"));
        }
    } else {
        results.append(ValidationResult("Playlist", "Item Count", false,
                                      "Playlist is empty",
                                      "Add test items to playlist for validation"));
    }
    
    // Check drag-drop mode
    QAbstractItemView::DragDropMode dragDropMode = listWidget->dragDropMode();
    if (dragDropMode != QAbstractItemView::NoDragDrop) {
        results.append(ValidationResult("Playlist", "Drag Drop Support", true, "Playlist supports drag and drop"));
    } else {
        results.append(ValidationResult("Playlist", "Drag Drop Support", false,
                                      "Playlist does not support drag and drop",
                                      "Enable drag and drop for playlist reordering"));
    }
    
    return results;
}

AccessibilityValidator::ValidationResult AccessibilityValidator::validateFocusHandling(QWidget* widget)
{
    if (!widget) {
        return ValidationResult("Focus", "Widget Check", false, "Widget is null");
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // Check focus policy
    Qt::FocusPolicy policy = widget->focusPolicy();
    if (policy != Qt::NoFocus) {
        return ValidationResult("Focus", "Focus Policy", true,
                              QString("%1 accepts focus").arg(widgetName));
    } else {
        return ValidationResult("Focus", "Focus Policy", false,
                              QString("%1 does not accept focus").arg(widgetName),
                              "Set appropriate focus policy for keyboard accessibility");
    }
}

AccessibilityValidator::ValidationResult AccessibilityValidator::validateKeyboardSupport(QWidget* widget)
{
    if (!widget) {
        return ValidationResult("Keyboard", "Widget Check", false, "Widget is null");
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // This is a basic check - in a real implementation, you might want to
    // simulate key events and check responses
    if (widget->focusPolicy() != Qt::NoFocus) {
        return ValidationResult("Keyboard", "Keyboard Support", true,
                              QString("%1 supports keyboard interaction").arg(widgetName));
    } else {
        return ValidationResult("Keyboard", "Keyboard Support", false,
                              QString("%1 does not support keyboard interaction").arg(widgetName),
                              "Implement keyboard support for accessibility");
    }
}

AccessibilityValidator::ValidationResult AccessibilityValidator::validateScreenReaderSupport(QWidget* widget)
{
    if (!widget) {
        return ValidationResult("Screen Reader", "Widget Check", false, "Widget is null");
    }
    
    QString widgetName = getWidgetTypeDescription(widget);
    
    // Check if widget has accessible name and description
    bool hasName = !widget->accessibleName().isEmpty();
    bool hasDescription = !widget->accessibleDescription().isEmpty();
    
    if (hasName || hasDescription) {
        return ValidationResult("Screen Reader", "Screen Reader Support", true,
                              QString("%1 has screen reader support").arg(widgetName));
    } else {
        return ValidationResult("Screen Reader", "Screen Reader Support", false,
                              QString("%1 lacks screen reader support").arg(widgetName),
                              "Add accessible name and description for screen readers");
    }
}

QList<QWidget*> AccessibilityValidator::findAccessibleWidgets()
{
    QList<QWidget*> widgets;
    
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        QWidgetList allWidgets = app->allWidgets();
        for (QWidget* widget : allWidgets) {
            if (shouldBeAccessible(widget)) {
                widgets.append(widget);
            }
        }
    }
    
    return widgets;
}

bool AccessibilityValidator::shouldBeAccessible(QWidget* widget)
{
    if (!widget || !widget->isVisible()) {
        return false;
    }
    
    // Check for specific widget types that should be accessible
    return qobject_cast<QTableView*>(widget) ||
           qobject_cast<QListWidget*>(widget) ||
           (widget->focusPolicy() != Qt::NoFocus && widget->isEnabled());
}

QString AccessibilityValidator::getWidgetTypeDescription(QWidget* widget)
{
    if (!widget) {
        return "Unknown Widget";
    }
    
    QString objectName = widget->objectName();
    QString className = widget->metaObject()->className();
    
    if (!objectName.isEmpty()) {
        return QString("%1 (%2)").arg(objectName).arg(className);
    } else {
        return className;
    }
}