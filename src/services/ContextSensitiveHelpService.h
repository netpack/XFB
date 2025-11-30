#ifndef CONTEXTSENSITIVEHELPSERVICE_H
#define CONTEXTSENSITIVEHELPSERVICE_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QTimer>
#include <QPointer>
#include <QKeyEvent>
#include <QApplication>
#include <QToolTip>
#include <QHelpEvent>
#include <QSettings>

class AccessibilityManager;
class AccessibleHelpSystem;

/**
 * @brief Context-sensitive help service for XFB
 * 
 * The ContextSensitiveHelpService provides intelligent help content delivery
 * based on the current focus and user context. It integrates with the main
 * AccessibleHelpSystem to provide targeted assistance and guided tutorials
 * that adapt to the user's current workflow.
 * 
 * Key features:
 * - Automatic context detection based on focused widget
 * - Workflow-aware help suggestions
 * - Progressive disclosure of advanced features
 * - Integration with guided tutorials
 * - Audio descriptions for complex operations
 * 
 * @example
 * @code
 * ContextSensitiveHelpService* service = new ContextSensitiveHelpService(accessibilityManager);
 * service->initialize();
 * service->registerContextHelp("player", "player_controls_overview");
 * service->enableAutoHelp(true);
 * @endcode
 * 
 * @see AccessibleHelpSystem, AccessibilityManager
 * @since XFB 2.0
 */
class ContextSensitiveHelpService : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Help delivery modes
     */
    enum class HelpMode {
        Manual,         ///< Help only on explicit request
        Automatic,      ///< Automatic help suggestions
        Progressive,    ///< Progressive help based on user experience
        Tutorial        ///< Guided tutorial mode
    };

    /**
     * @brief Context information for help delivery
     */
    struct HelpContext {
        QString widgetClass;        ///< Widget class name
        QString objectName;         ///< Widget object name
        QString parentContext;      ///< Parent widget context
        QString currentOperation;   ///< Current operation being performed
        QStringList recentActions;  ///< Recent user actions
        int focusTime;             ///< Time spent in current context (seconds)
        bool isFirstVisit;         ///< Whether this is first time in this context
        
        HelpContext() : focusTime(0), isFirstVisit(true) {}
    };

    /**
     * @brief Context help mapping entry
     */
    struct ContextHelpMapping {
        QString contextId;          ///< Context identifier
        QString helpContentId;      ///< Associated help content ID
        QString tutorialId;         ///< Associated tutorial ID (optional)
        QString audioDescription;   ///< Brief audio description
        QStringList prerequisites;  ///< Required knowledge prerequisites
        int priority;              ///< Help priority (higher = more important)
        bool autoTrigger;          ///< Whether to auto-trigger help
        int autoTriggerDelay;      ///< Delay before auto-trigger (seconds)
        
        ContextHelpMapping() : priority(0), autoTrigger(false), autoTriggerDelay(5) {}
    };

    /**
     * @brief Guided instruction step
     */
    struct GuidedInstruction {
        QString instruction;        ///< Step instruction text
        QString audioDescription;   ///< Audio description for screen readers
        QString targetWidget;       ///< Widget to highlight/focus
        QString expectedAction;     ///< Expected user action
        QStringList validationKeys; ///< Keys that validate completion
        int timeoutSeconds;        ///< Timeout for this step
        bool waitForCompletion;    ///< Whether to wait for user completion
        
        GuidedInstruction() : timeoutSeconds(30), waitForCompletion(true) {}
    };

    /**
     * @brief Workflow-based help sequence
     */
    struct WorkflowHelp {
        QString workflowId;         ///< Workflow identifier
        QString title;              ///< Workflow title
        QString description;        ///< Workflow description
        QList<GuidedInstruction> steps; ///< Workflow steps
        QStringList triggerContexts; ///< Contexts that can trigger this workflow
        QString completionMessage;  ///< Message on workflow completion
        
        WorkflowHelp() {}
    };

    explicit ContextSensitiveHelpService(AccessibilityManager* accessibilityManager, QObject* parent = nullptr);
    ~ContextSensitiveHelpService() override;

    /**
     * @brief Initialize the context-sensitive help service
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the service
     */
    void shutdown();

    /**
     * @brief Set the help system reference
     * @param helpSystem Pointer to the main help system
     */
    void setHelpSystem(AccessibleHelpSystem* helpSystem);

    /**
     * @brief Set the help delivery mode
     * @param mode Help delivery mode
     */
    void setHelpMode(HelpMode mode);

    /**
     * @brief Get the current help delivery mode
     * @return Current help mode
     */
    HelpMode helpMode() const { return m_helpMode; }

    /**
     * @brief Register context help mapping
     * @param contextId Context identifier
     * @param mapping Help mapping information
     */
    void registerContextHelp(const QString& contextId, const ContextHelpMapping& mapping);

    /**
     * @brief Register workflow help sequence
     * @param workflow Workflow help information
     */
    void registerWorkflowHelp(const WorkflowHelp& workflow);

    /**
     * @brief Enable or disable automatic help suggestions
     * @param enabled Whether to enable auto help
     */
    void enableAutoHelp(bool enabled);

    /**
     * @brief Check if auto help is enabled
     * @return true if auto help is enabled
     */
    bool isAutoHelpEnabled() const { return m_autoHelpEnabled; }

    /**
     * @brief Get context help for a widget
     * @param widget Widget to get help for
     * @return Context help mapping, or default if not found
     */
    ContextHelpMapping getContextHelp(QWidget* widget) const;

    /**
     * @brief Show context help for current focus
     * @return true if help was shown
     */
    bool showContextHelp();

    /**
     * @brief Show context help for specific widget
     * @param widget Widget to show help for
     * @return true if help was shown
     */
    bool showContextHelp(QWidget* widget);

    /**
     * @brief Start guided workflow for current context
     * @return true if workflow was started
     */
    bool startGuidedWorkflow();

    /**
     * @brief Start specific guided workflow
     * @param workflowId Workflow identifier
     * @return true if workflow was started
     */
    bool startGuidedWorkflow(const QString& workflowId);

    /**
     * @brief Check if help is available for current context
     * @return true if context help is available
     */
    bool hasContextHelp() const;

    /**
     * @brief Check if help is available for specific widget
     * @param widget Widget to check
     * @return true if context help is available
     */
    bool hasContextHelp(QWidget* widget) const;

    /**
     * @brief Get current help context information
     * @return Current context information
     */
    HelpContext getCurrentContext() const;

public slots:
    /**
     * @brief Handle focus change events
     * @param old Previously focused widget
     * @param now Currently focused widget
     */
    void onFocusChanged(QWidget* old, QWidget* now);

    /**
     * @brief Handle context help request
     */
    void onContextHelpRequested();

    /**
     * @brief Handle workflow help request
     */
    void onWorkflowHelpRequested();

    /**
     * @brief Handle user action for workflow validation
     * @param action Action identifier
     */
    void onUserAction(const QString& action);

signals:
    /**
     * @brief Emitted when context help is shown
     * @param contextId Context identifier
     * @param helpContentId Help content ID
     */
    void contextHelpShown(const QString& contextId, const QString& helpContentId);

    /**
     * @brief Emitted when guided workflow starts
     * @param workflowId Workflow identifier
     */
    void guidedWorkflowStarted(const QString& workflowId);

    /**
     * @brief Emitted when workflow step changes
     * @param workflowId Workflow identifier
     * @param stepIndex Current step index
     * @param instruction Step instruction
     */
    void workflowStepChanged(const QString& workflowId, int stepIndex, const QString& instruction);

    /**
     * @brief Emitted when guided workflow completes
     * @param workflowId Workflow identifier
     */
    void guidedWorkflowCompleted(const QString& workflowId);

    /**
     * @brief Emitted when auto help suggestion is available
     * @param contextId Context identifier
     * @param suggestion Help suggestion text
     */
    void autoHelpSuggestion(const QString& contextId, const QString& suggestion);

protected:
    /**
     * @brief Event filter for monitoring user interactions
     * @param obj Object that received the event
     * @param event Event to process
     * @return true if event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief Handle auto help timer timeout
     */
    void onAutoHelpTimer();

    /**
     * @brief Handle workflow step timer timeout
     */
    void onWorkflowStepTimer();

    /**
     * @brief Handle context focus timer
     */
    void onContextFocusTimer();

private:
    /**
     * @brief Determine context ID for a widget
     * @param widget Widget to analyze
     * @return Context identifier
     */
    QString determineContextId(QWidget* widget) const;

    /**
     * @brief Update current context information
     * @param widget Currently focused widget
     */
    void updateCurrentContext(QWidget* widget);

    /**
     * @brief Check if auto help should be triggered
     * @param contextId Context identifier
     * @return true if auto help should be triggered
     */
    bool shouldTriggerAutoHelp(const QString& contextId) const;

    /**
     * @brief Trigger auto help for current context
     */
    void triggerAutoHelp();

    /**
     * @brief Load default context help mappings
     */
    void loadDefaultContextMappings();

    /**
     * @brief Load default workflow help sequences
     */
    void loadDefaultWorkflowHelp();

    /**
     * @brief Find workflow for current context
     * @return Workflow ID, or empty string if not found
     */
    QString findWorkflowForContext() const;

    /**
     * @brief Execute workflow step
     * @param workflow Workflow information
     * @param stepIndex Step index to execute
     */
    void executeWorkflowStep(const WorkflowHelp& workflow, int stepIndex);

    /**
     * @brief Validate workflow step completion
     * @param step Workflow step
     * @param action User action
     * @return true if step is completed
     */
    bool validateWorkflowStep(const GuidedInstruction& step, const QString& action) const;

    /**
     * @brief Announce workflow instruction
     * @param instruction Workflow instruction
     */
    void announceWorkflowInstruction(const GuidedInstruction& instruction);

    /**
     * @brief Get widget hierarchy path
     * @param widget Widget to analyze
     * @return Hierarchy path as string
     */
    QString getWidgetHierarchyPath(QWidget* widget) const;

    /**
     * @brief Track user action for context analysis
     * @param action Action identifier
     */
    void trackUserAction(const QString& action);

    /**
     * @brief Check if user has prerequisite knowledge
     * @param prerequisites List of prerequisite identifiers
     * @return true if user has required knowledge
     */
    bool hasPrerequisiteKnowledge(const QStringList& prerequisites) const;

    // Member variables
    AccessibilityManager* m_accessibilityManager;
    AccessibleHelpSystem* m_helpSystem;
    
    // Configuration
    HelpMode m_helpMode;
    bool m_autoHelpEnabled;
    
    // Context tracking
    HelpContext m_currentContext;
    QPointer<QWidget> m_currentWidget;
    QString m_currentContextId;
    QHash<QString, qint64> m_contextVisitTimes;
    QHash<QString, int> m_contextVisitCounts;
    
    // Help mappings
    QHash<QString, ContextHelpMapping> m_contextMappings;
    QHash<QString, WorkflowHelp> m_workflowHelp;
    
    // Workflow state
    QString m_currentWorkflowId;
    int m_currentWorkflowStep;
    bool m_workflowActive;
    QTimer* m_workflowStepTimer;
    
    // Auto help state
    QTimer* m_autoHelpTimer;
    QTimer* m_contextFocusTimer;
    QString m_pendingAutoHelpContext;
    
    // User tracking
    QStringList m_recentActions;
    QHash<QString, bool> m_completedTutorials;
    QHash<QString, int> m_featureUsageCounts;
    
    // State tracking
    bool m_initialized;
    qint64 m_contextStartTime;
    
    // Configuration constants
    static constexpr int AUTO_HELP_DELAY_MS = 5000;
    static constexpr int CONTEXT_FOCUS_UPDATE_MS = 1000;
    static constexpr int WORKFLOW_STEP_TIMEOUT_MS = 30000;
    static constexpr int MAX_RECENT_ACTIONS = 10;
    static constexpr const char* SETTINGS_GROUP = "ContextHelp";
};

Q_DECLARE_METATYPE(ContextSensitiveHelpService::HelpMode)

#endif // CONTEXTSENSITIVEHELPSERVICE_H