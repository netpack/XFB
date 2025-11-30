#ifndef ACCESSIBILITYVALIDATOR_H
#define ACCESSIBILITYVALIDATOR_H

#include <QObject>
#include <QWidget>
#include <QTableView>
#include <QListWidget>
#include <QStringList>
#include <QTimer>

class AccessibilityManager;

/**
 * @brief Validates accessibility implementation for user acceptance testing
 * 
 * This class provides automated validation of accessibility features to support
 * user acceptance testing. It checks for proper implementation of accessibility
 * interfaces, keyboard navigation, and screen reader compatibility.
 * 
 * @since XFB 2.0
 */
class AccessibilityValidator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Validation result structure
     */
    struct ValidationResult {
        QString component;
        QString testName;
        bool passed;
        QString message;
        QString recommendation;
        
        ValidationResult(const QString& comp, const QString& test, bool pass, 
                        const QString& msg, const QString& rec = QString())
            : component(comp), testName(test), passed(pass), message(msg), recommendation(rec) {}
    };

    explicit AccessibilityValidator(AccessibilityManager* manager, QObject* parent = nullptr);
    ~AccessibilityValidator() override;

    /**
     * @brief Run comprehensive accessibility validation
     * @return List of validation results
     */
    QList<ValidationResult> runFullValidation();

    /**
     * @brief Validate table view accessibility
     * @param tableView The table view to validate
     * @return List of validation results for the table
     */
    QList<ValidationResult> validateTableView(QTableView* tableView);

    /**
     * @brief Validate playlist accessibility
     * @param listWidget The list widget to validate
     * @return List of validation results for the playlist
     */
    QList<ValidationResult> validatePlaylist(QListWidget* listWidget);

    /**
     * @brief Validate keyboard navigation
     * @param widget The widget to validate
     * @return List of validation results for keyboard navigation
     */
    QList<ValidationResult> validateKeyboardNavigation(QWidget* widget);

    /**
     * @brief Generate validation report
     * @param results The validation results
     * @return Formatted report string
     */
    QString generateReport(const QList<ValidationResult>& results);

    /**
     * @brief Check if accessibility is properly enabled
     * @return true if accessibility is properly configured
     */
    bool isAccessibilityEnabled() const;

    /**
     * @brief Get summary of validation results
     * @param results The validation results
     * @return Summary statistics
     */
    struct ValidationSummary {
        int totalTests;
        int passedTests;
        int failedTests;
        int criticalIssues;
        double passRate;
    };
    ValidationSummary getSummary(const QList<ValidationResult>& results);

public slots:
    /**
     * @brief Run validation on all accessible widgets
     */
    void validateAllWidgets();

signals:
    /**
     * @brief Emitted when validation is complete
     * @param results The validation results
     */
    void validationComplete(const QList<AccessibilityValidator::ValidationResult>& results);

    /**
     * @brief Emitted during validation progress
     * @param progress Progress percentage (0-100)
     * @param currentTest Description of current test
     */
    void validationProgress(int progress, const QString& currentTest);

private:
    /**
     * @brief Validate accessible interface implementation
     * @param widget The widget to check
     * @return Validation result
     */
    ValidationResult validateAccessibleInterface(QWidget* widget);

    /**
     * @brief Validate accessible properties
     * @param widget The widget to check
     * @return Validation result
     */
    ValidationResult validateAccessibleProperties(QWidget* widget);

    /**
     * @brief Validate table-specific accessibility features
     * @param tableView The table view to check
     * @return List of validation results
     */
    QList<ValidationResult> validateTableSpecificFeatures(QTableView* tableView);

    /**
     * @brief Validate playlist-specific accessibility features
     * @param listWidget The list widget to check
     * @return List of validation results
     */
    QList<ValidationResult> validatePlaylistSpecificFeatures(QListWidget* listWidget);

    /**
     * @brief Check if widget has proper focus handling
     * @param widget The widget to check
     * @return Validation result
     */
    ValidationResult validateFocusHandling(QWidget* widget);

    /**
     * @brief Check if widget supports keyboard navigation
     * @param widget The widget to check
     * @return Validation result
     */
    ValidationResult validateKeyboardSupport(QWidget* widget);

    /**
     * @brief Validate screen reader announcements
     * @param widget The widget to check
     * @return Validation result
     */
    ValidationResult validateScreenReaderSupport(QWidget* widget);

    /**
     * @brief Find all accessible widgets in the application
     * @return List of widgets that should be accessible
     */
    QList<QWidget*> findAccessibleWidgets();

    /**
     * @brief Check if widget type should be accessible
     * @param widget The widget to check
     * @return true if widget should have accessibility features
     */
    bool shouldBeAccessible(QWidget* widget);

    /**
     * @brief Get widget type description
     * @param widget The widget to describe
     * @return Human-readable widget type
     */
    QString getWidgetTypeDescription(QWidget* widget);

    AccessibilityManager* m_accessibilityManager;
    QTimer* m_validationTimer;
    QList<ValidationResult> m_currentResults;
    int m_currentProgress;
};

Q_DECLARE_METATYPE(AccessibilityValidator::ValidationResult)

#endif // ACCESSIBILITYVALIDATOR_H