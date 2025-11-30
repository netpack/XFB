#include "ContextSensitiveHelpService.h"
#include "AccessibilityManager.h"
#include "AccessibleHelpSystem.h"
#include <QApplication>
#include <QDateTime>
#include <QSettings>
#include <QDebug>
#include <QMainWindow>
#include <QDialog>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>

ContextSensitiveHelpService::ContextSensitiveHelpService(AccessibilityManager* accessibilityManager, QObject* parent)
    : QObject(parent)
    , m_accessibilityManager(accessibilityManager)
    , m_helpSystem(nullptr)
    , m_helpMode(HelpMode::Manual)
    , m_autoHelpEnabled(false)
    , m_currentWidget(nullptr)
    , m_currentWorkflowStep(0)
    , m_workflowActive(false)
    , m_workflowStepTimer(nullptr)
    , m_autoHelpTimer(nullptr)
    , m_contextFocusTimer(nullptr)
    , m_initialized(false)
    , m_contextStartTime(0)
{
    // Initialize timers
    m_autoHelpTimer = new QTimer(this);
    m_autoHelpTimer->setSingleShot(true);
    connect(m_autoHelpTimer, &QTimer::timeout,
            this, &ContextSensitiveHelpService::onAutoHelpTimer);

    m_workflowStepTimer = new QTimer(this);
    m_workflowStepTimer->setSingleShot(true);
    connect(m_workflowStepTimer, &QTimer::timeout,
            this, &ContextSensitiveHelpService::onWorkflowStepTimer);

    m_contextFocusTimer = new QTimer(this);
    m_contextFocusTimer->setSingleShot(false);
    m_contextFocusTimer->setInterval(CONTEXT_FOCUS_UPDATE_MS);
    connect(m_contextFocusTimer, &QTimer::timeout,
            this, &ContextSensitiveHelpService::onContextFocusTimer);
}

ContextSensitiveHelpService::~ContextSensitiveHelpService()
{
    shutdown();
}

bool ContextSensitiveHelpService::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "ContextSensitiveHelpService: Initializing context-sensitive help service";

    // Load default context mappings
    loadDefaultContextMappings();
    
    // Load default workflow help
    loadDefaultWorkflowHelp();
    
    // Connect to application focus changes
    if (QApplication::instance()) {
        connect(QApplication::instance(), &QApplication::focusChanged,
                this, &ContextSensitiveHelpService::onFocusChanged);
    }
    
    // Install event filter on application
    if (QApplication::instance()) {
        QApplication::instance()->installEventFilter(this);
    }
    
    // Load settings
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    m_helpMode = static_cast<HelpMode>(settings.value("HelpMode", static_cast<int>(HelpMode::Manual)).toInt());
    m_autoHelpEnabled = settings.value("AutoHelpEnabled", false).toBool();
    settings.endGroup();

    m_initialized = true;
    qDebug() << "ContextSensitiveHelpService: Initialization completed successfully";
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            "Context-sensitive help service initialized.",
            AccessibilityManager::Priority::Low
        );
    }
    
    return true;
}

void ContextSensitiveHelpService::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "ContextSensitiveHelpService: Shutting down context-sensitive help service";

    // Save settings
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    settings.setValue("HelpMode", static_cast<int>(m_helpMode));
    settings.setValue("AutoHelpEnabled", m_autoHelpEnabled);
    settings.endGroup();

    // Stop timers
    if (m_autoHelpTimer) {
        m_autoHelpTimer->stop();
    }
    if (m_workflowStepTimer) {
        m_workflowStepTimer->stop();
    }
    if (m_contextFocusTimer) {
        m_contextFocusTimer->stop();
    }
    
    // Remove event filter
    if (QApplication::instance()) {
        QApplication::instance()->removeEventFilter(this);
    }
    
    // Clear data
    m_contextMappings.clear();
    m_workflowHelp.clear();
    m_contextVisitTimes.clear();
    m_contextVisitCounts.clear();
    m_recentActions.clear();
    m_completedTutorials.clear();
    m_featureUsageCounts.clear();
    
    m_initialized = false;
}

void ContextSensitiveHelpService::setHelpSystem(AccessibleHelpSystem* helpSystem)
{
    m_helpSystem = helpSystem;
}

void ContextSensitiveHelpService::setHelpMode(HelpMode mode)
{
    if (m_helpMode != mode) {
        m_helpMode = mode;
        
        // Adjust auto help based on mode
        if (mode == HelpMode::Automatic || mode == HelpMode::Progressive) {
            enableAutoHelp(true);
        } else if (mode == HelpMode::Manual) {
            enableAutoHelp(false);
        }
        
        qDebug() << "ContextSensitiveHelpService: Help mode changed to" << static_cast<int>(mode);
    }
}

void ContextSensitiveHelpService::registerContextHelp(const QString& contextId, const ContextHelpMapping& mapping)
{
    if (contextId.isEmpty()) {
        qWarning() << "ContextSensitiveHelpService: Cannot register context help with empty ID";
        return;
    }

    m_contextMappings[contextId] = mapping;
    qDebug() << "ContextSensitiveHelpService: Registered context help for" << contextId;
}

void ContextSensitiveHelpService::registerWorkflowHelp(const WorkflowHelp& workflow)
{
    if (workflow.workflowId.isEmpty()) {
        qWarning() << "ContextSensitiveHelpService: Cannot register workflow help with empty ID";
        return;
    }

    m_workflowHelp[workflow.workflowId] = workflow;
    qDebug() << "ContextSensitiveHelpService: Registered workflow help for" << workflow.workflowId;
}

void ContextSensitiveHelpService::enableAutoHelp(bool enabled)
{
    if (m_autoHelpEnabled != enabled) {
        m_autoHelpEnabled = enabled;
        
        if (enabled) {
            // Start context focus timer
            m_contextFocusTimer->start();
        } else {
            // Stop timers
            m_autoHelpTimer->stop();
            m_contextFocusTimer->stop();
        }
        
        qDebug() << "ContextSensitiveHelpService: Auto help" << (enabled ? "enabled" : "disabled");
    }
}

ContextSensitiveHelpService::ContextHelpMapping ContextSensitiveHelpService::getContextHelp(QWidget* widget) const
{
    if (!widget) {
        return ContextHelpMapping();
    }

    QString contextId = determineContextId(widget);
    return m_contextMappings.value(contextId, ContextHelpMapping());
}

bool ContextSensitiveHelpService::showContextHelp()
{
    QWidget* focusWidget = QApplication::focusWidget();
    return showContextHelp(focusWidget);
}

bool ContextSensitiveHelpService::showContextHelp(QWidget* widget)
{
    if (!widget || !m_helpSystem) {
        return false;
    }

    QString contextId = determineContextId(widget);
    ContextHelpMapping mapping = m_contextMappings.value(contextId);
    
    if (mapping.helpContentId.isEmpty()) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                "No context help available for this control.",
                AccessibilityManager::Priority::Normal
            );
        }
        return false;
    }

    // Check prerequisites
    if (!hasPrerequisiteKnowledge(mapping.prerequisites)) {
        // Suggest prerequisite tutorial instead
        if (!mapping.prerequisites.isEmpty()) {
            QString prerequisite = mapping.prerequisites.first();
            if (m_accessibilityManager) {
                m_accessibilityManager->announceMessage(
                    QString("This feature requires basic knowledge. Would you like to start the %1 tutorial first?")
                        .arg(prerequisite),
                    AccessibilityManager::Priority::Normal
                );
            }
        }
        return false;
    }

    // Show the help content
    bool success = m_helpSystem->showContextHelp(widget);
    
    if (success) {
        // Announce audio description if available
        if (!mapping.audioDescription.isEmpty() && m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                mapping.audioDescription,
                AccessibilityManager::Priority::Normal
            );
        }
        
        // Track usage
        m_featureUsageCounts[contextId]++;
        
        emit contextHelpShown(contextId, mapping.helpContentId);
    }
    
    return success;
}

bool ContextSensitiveHelpService::startGuidedWorkflow()
{
    QString workflowId = findWorkflowForContext();
    return startGuidedWorkflow(workflowId);
}

bool ContextSensitiveHelpService::startGuidedWorkflow(const QString& workflowId)
{
    if (workflowId.isEmpty() || !m_workflowHelp.contains(workflowId)) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                "No guided workflow available for this context.",
                AccessibilityManager::Priority::Normal
            );
        }
        return false;
    }

    const WorkflowHelp& workflow = m_workflowHelp[workflowId];
    
    if (workflow.steps.isEmpty()) {
        qWarning() << "ContextSensitiveHelpService: Workflow has no steps:" << workflowId;
        return false;
    }

    m_currentWorkflowId = workflowId;
    m_currentWorkflowStep = 0;
    m_workflowActive = true;

    // Execute first step
    executeWorkflowStep(workflow, 0);

    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            QString("Started guided workflow: %1. %2 steps total.")
                .arg(workflow.title)
                .arg(workflow.steps.size()),
            AccessibilityManager::Priority::High
        );
    }

    emit guidedWorkflowStarted(workflowId);
    return true;
}

bool ContextSensitiveHelpService::hasContextHelp() const
{
    QWidget* focusWidget = QApplication::focusWidget();
    return hasContextHelp(focusWidget);
}

bool ContextSensitiveHelpService::hasContextHelp(QWidget* widget) const
{
    if (!widget) {
        return false;
    }

    QString contextId = determineContextId(widget);
    return m_contextMappings.contains(contextId) && 
           !m_contextMappings[contextId].helpContentId.isEmpty();
}

ContextSensitiveHelpService::HelpContext ContextSensitiveHelpService::getCurrentContext() const
{
    return m_currentContext;
}

void ContextSensitiveHelpService::onFocusChanged(QWidget* old, QWidget* now)
{
    Q_UNUSED(old)
    
    if (now != m_currentWidget) {
        updateCurrentContext(now);
        
        // Reset auto help timer if enabled
        if (m_autoHelpEnabled && now) {
            QString contextId = determineContextId(now);
            if (shouldTriggerAutoHelp(contextId)) {
                m_pendingAutoHelpContext = contextId;
                m_autoHelpTimer->start(AUTO_HELP_DELAY_MS);
            }
        }
    }
}

void ContextSensitiveHelpService::onContextHelpRequested()
{
    showContextHelp();
}

void ContextSensitiveHelpService::onWorkflowHelpRequested()
{
    startGuidedWorkflow();
}

void ContextSensitiveHelpService::onUserAction(const QString& action)
{
    trackUserAction(action);
    
    // Check if this action completes current workflow step
    if (m_workflowActive && !m_currentWorkflowId.isEmpty()) {
        const WorkflowHelp& workflow = m_workflowHelp[m_currentWorkflowId];
        
        if (m_currentWorkflowStep < workflow.steps.size()) {
            const GuidedInstruction& step = workflow.steps[m_currentWorkflowStep];
            
            if (validateWorkflowStep(step, action)) {
                // Move to next step
                m_currentWorkflowStep++;
                
                if (m_currentWorkflowStep < workflow.steps.size()) {
                    executeWorkflowStep(workflow, m_currentWorkflowStep);
                } else {
                    // Workflow completed
                    m_workflowActive = false;
                    m_workflowStepTimer->stop();
                    
                    if (m_accessibilityManager) {
                        m_accessibilityManager->announceMessage(
                            workflow.completionMessage.isEmpty() ? 
                                QString("Workflow completed: %1").arg(workflow.title) :
                                workflow.completionMessage,
                            AccessibilityManager::Priority::High
                        );
                    }
                    
                    // Mark as completed
                    m_completedTutorials[m_currentWorkflowId] = true;
                    
                    emit guidedWorkflowCompleted(m_currentWorkflowId);
                }
            }
        }
    }
}

bool ContextSensitiveHelpService::eventFilter(QObject* obj, QEvent* event)
{
    if (!m_initialized) {
        return false;
    }

    // Track user interactions for context analysis
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        QString action = QString("key_%1").arg(keyEvent->key());
        trackUserAction(action);
    } else if (event->type() == QEvent::MouseButtonPress) {
        trackUserAction("mouse_click");
    }

    return false; // Don't consume events
}

void ContextSensitiveHelpService::onAutoHelpTimer()
{
    if (!m_pendingAutoHelpContext.isEmpty() && m_autoHelpEnabled) {
        triggerAutoHelp();
    }
}

void ContextSensitiveHelpService::onWorkflowStepTimer()
{
    if (m_workflowActive && !m_currentWorkflowId.isEmpty()) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                "Workflow step timed out. You can continue at your own pace or ask for help.",
                AccessibilityManager::Priority::Normal
            );
        }
    }
}

void ContextSensitiveHelpService::onContextFocusTimer()
{
    if (m_currentWidget && m_contextStartTime > 0) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        m_currentContext.focusTime = (currentTime - m_contextStartTime) / 1000;
    }
}

QString ContextSensitiveHelpService::determineContextId(QWidget* widget) const
{
    if (!widget) {
        return QString();
    }

    // Check object name first
    QString objectName = widget->objectName();
    if (!objectName.isEmpty()) {
        return objectName;
    }
    
    // Check class name
    QString className = widget->metaObject()->className();
    
    // Add parent context for more specific identification
    QWidget* parent = widget->parentWidget();
    if (parent) {
        QString parentClass = parent->metaObject()->className();
        QString parentName = parent->objectName();
        
        if (!parentName.isEmpty()) {
            return QString("%1_in_%2").arg(className, parentName);
        } else {
            return QString("%1_in_%2").arg(className, parentClass);
        }
    }
    
    return className;
}

void ContextSensitiveHelpService::updateCurrentContext(QWidget* widget)
{
    m_currentWidget = widget;
    m_contextStartTime = QDateTime::currentMSecsSinceEpoch();
    
    if (widget) {
        QString contextId = determineContextId(widget);
        
        // Update context information
        m_currentContext.widgetClass = widget->metaObject()->className();
        m_currentContext.objectName = widget->objectName();
        m_currentContext.focusTime = 0;
        
        // Check if this is first visit
        m_currentContext.isFirstVisit = !m_contextVisitTimes.contains(contextId);
        
        // Update visit tracking
        m_contextVisitTimes[contextId] = m_contextStartTime;
        m_contextVisitCounts[contextId]++;
        
        // Update parent context
        QWidget* parent = widget->parentWidget();
        if (parent) {
            m_currentContext.parentContext = determineContextId(parent);
        } else {
            m_currentContext.parentContext.clear();
        }
        
        m_currentContextId = contextId;
        
        qDebug() << "ContextSensitiveHelpService: Context updated to" << contextId;
    } else {
        m_currentContext = HelpContext();
        m_currentContextId.clear();
    }
}

bool ContextSensitiveHelpService::shouldTriggerAutoHelp(const QString& contextId) const
{
    if (!m_autoHelpEnabled || contextId.isEmpty()) {
        return false;
    }

    // Check if we have help for this context
    if (!m_contextMappings.contains(contextId)) {
        return false;
    }

    const ContextHelpMapping& mapping = m_contextMappings[contextId];
    
    // Check if auto trigger is enabled for this context
    if (!mapping.autoTrigger) {
        return false;
    }

    // Check help mode
    if (m_helpMode == HelpMode::Manual) {
        return false;
    }

    // For progressive mode, check if user is new to this context
    if (m_helpMode == HelpMode::Progressive) {
        int visitCount = m_contextVisitCounts.value(contextId, 0);
        if (visitCount > 2) { // Don't auto-help after a few visits
            return false;
        }
    }

    return true;
}

void ContextSensitiveHelpService::triggerAutoHelp()
{
    if (m_pendingAutoHelpContext.isEmpty()) {
        return;
    }

    QString contextId = m_pendingAutoHelpContext;
    m_pendingAutoHelpContext.clear();

    const ContextHelpMapping& mapping = m_contextMappings[contextId];
    
    QString suggestion;
    if (!mapping.audioDescription.isEmpty()) {
        suggestion = mapping.audioDescription;
    } else {
        suggestion = QString("Help is available for this control. Press Shift+F1 for context help.");
    }
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            suggestion,
            AccessibilityManager::Priority::Low
        );
    }
    
    emit autoHelpSuggestion(contextId, suggestion);
}

void ContextSensitiveHelpService::loadDefaultContextMappings()
{
    // Player controls context help
    ContextHelpMapping playerMapping;
    playerMapping.contextId = "player";
    playerMapping.helpContentId = "player_controls_overview";
    playerMapping.audioDescription = "Player controls. Use Space to play/pause, Ctrl+Up/Down for volume.";
    playerMapping.priority = 90;
    playerMapping.autoTrigger = true;
    playerMapping.autoTriggerDelay = 3;
    registerContextHelp("player", playerMapping);

    // Database grid context help
    ContextHelpMapping databaseMapping;
    databaseMapping.contextId = "QTableView";
    databaseMapping.helpContentId = "database_management_overview";
    databaseMapping.audioDescription = "Database grid. Use arrow keys to navigate, F2 to edit, Enter to confirm.";
    databaseMapping.priority = 85;
    databaseMapping.autoTrigger = true;
    databaseMapping.autoTriggerDelay = 5;
    registerContextHelp("QTableView", databaseMapping);

    // Playlist context help
    ContextHelpMapping playlistMapping;
    playlistMapping.contextId = "QListWidget";
    playlistMapping.helpContentId = "playlist_management_overview";
    playlistMapping.audioDescription = "Playlist. Use arrow keys to navigate, Ctrl+C/V to copy/paste tracks.";
    playlistMapping.priority = 80;
    playlistMapping.autoTrigger = true;
    registerContextHelp("QListWidget", playlistMapping);

    // Button context help
    ContextHelpMapping buttonMapping;
    buttonMapping.contextId = "QPushButton";
    buttonMapping.helpContentId = "general_controls_help";
    buttonMapping.audioDescription = "Button control. Press Enter or Space to activate.";
    buttonMapping.priority = 50;
    buttonMapping.autoTrigger = false; // Don't auto-trigger for common controls
    registerContextHelp("QPushButton", buttonMapping);

    // Text input context help
    ContextHelpMapping textMapping;
    textMapping.contextId = "QLineEdit";
    textMapping.helpContentId = "text_input_help";
    textMapping.audioDescription = "Text input field. Type to enter text, Tab to move to next field.";
    textMapping.priority = 60;
    textMapping.autoTrigger = false;
    registerContextHelp("QLineEdit", textMapping);
}

void ContextSensitiveHelpService::loadDefaultWorkflowHelp()
{
    // First-time user workflow
    WorkflowHelp firstTimeWorkflow;
    firstTimeWorkflow.workflowId = "first_time_user";
    firstTimeWorkflow.title = "First Time User Guide";
    firstTimeWorkflow.description = "Complete guide for new XFB users";
    firstTimeWorkflow.triggerContexts = {"player", "QTableView"};
    firstTimeWorkflow.completionMessage = "Congratulations! You've completed the basic XFB tutorial.";
    
    GuidedInstruction step1;
    step1.instruction = "Welcome to XFB! Let's start by learning basic navigation. Press Tab to move between controls.";
    step1.audioDescription = "Press Tab key to navigate between interface elements.";
    step1.expectedAction = "key_16777217"; // Tab key
    step1.validationKeys = {"key_16777217"};
    firstTimeWorkflow.steps.append(step1);
    
    GuidedInstruction step2;
    step2.instruction = "Great! Now press F1 to open the help system whenever you need assistance.";
    step2.audioDescription = "Press F1 to open help documentation.";
    step2.expectedAction = "key_16777264"; // F1 key
    step2.validationKeys = {"key_16777264"};
    firstTimeWorkflow.steps.append(step2);
    
    GuidedInstruction step3;
    step3.instruction = "Excellent! You can also press Shift+F1 for context-sensitive help on any control.";
    step3.audioDescription = "Press Shift+F1 for context-specific help.";
    step3.expectedAction = "context_help";
    step3.validationKeys = {"context_help"};
    firstTimeWorkflow.steps.append(step3);
    
    registerWorkflowHelp(firstTimeWorkflow);

    // Player operation workflow
    WorkflowHelp playerWorkflow;
    playerWorkflow.workflowId = "player_operation";
    playerWorkflow.title = "Player Operation Guide";
    playerWorkflow.description = "Learn to control audio playback";
    playerWorkflow.triggerContexts = {"player"};
    playerWorkflow.completionMessage = "You now know the basic player controls!";
    
    GuidedInstruction playerStep1;
    playerStep1.instruction = "Let's learn player controls. First, press Space to start or pause playback.";
    playerStep1.audioDescription = "Press Space bar to toggle play and pause.";
    playerStep1.expectedAction = "play_pause";
    playerStep1.validationKeys = {"play_pause", "key_32"}; // Space key
    playerWorkflow.steps.append(playerStep1);
    
    GuidedInstruction playerStep2;
    playerStep2.instruction = "Good! Now try adjusting volume with Ctrl+Up or Ctrl+Down.";
    playerStep2.audioDescription = "Use Ctrl+Up and Ctrl+Down to adjust volume.";
    playerStep2.expectedAction = "volume_up";
    playerStep2.validationKeys = {"volume_up", "volume_down"};
    playerWorkflow.steps.append(playerStep2);
    
    registerWorkflowHelp(playerWorkflow);
}

QString ContextSensitiveHelpService::findWorkflowForContext() const
{
    if (m_currentContextId.isEmpty()) {
        return QString();
    }

    // Find workflow that matches current context
    for (auto it = m_workflowHelp.constBegin(); it != m_workflowHelp.constEnd(); ++it) {
        const WorkflowHelp& workflow = it.value();
        
        if (workflow.triggerContexts.contains(m_currentContextId)) {
            // Check if user has completed this workflow
            if (!m_completedTutorials.value(it.key(), false)) {
                return it.key();
            }
        }
    }
    
    return QString();
}

void ContextSensitiveHelpService::executeWorkflowStep(const WorkflowHelp& workflow, int stepIndex)
{
    if (stepIndex < 0 || stepIndex >= workflow.steps.size()) {
        return;
    }

    const GuidedInstruction& step = workflow.steps[stepIndex];
    
    // Announce the instruction
    announceWorkflowInstruction(step);
    
    // Start step timer if needed
    if (step.waitForCompletion && step.timeoutSeconds > 0) {
        m_workflowStepTimer->start(step.timeoutSeconds * 1000);
    }
    
    emit workflowStepChanged(workflow.workflowId, stepIndex, step.instruction);
}

bool ContextSensitiveHelpService::validateWorkflowStep(const GuidedInstruction& step, const QString& action) const
{
    if (step.expectedAction == action) {
        return true;
    }
    
    // Check validation keys
    for (const QString& validKey : step.validationKeys) {
        if (validKey == action) {
            return true;
        }
    }
    
    return false;
}

void ContextSensitiveHelpService::announceWorkflowInstruction(const GuidedInstruction& instruction)
{
    if (!m_accessibilityManager) {
        return;
    }

    QString announcement = instruction.audioDescription.isEmpty() ? 
                          instruction.instruction : 
                          instruction.audioDescription;
    
    m_accessibilityManager->announceMessage(
        announcement,
        AccessibilityManager::Priority::High
    );
}

QString ContextSensitiveHelpService::getWidgetHierarchyPath(QWidget* widget) const
{
    if (!widget) {
        return QString();
    }

    QStringList path;
    QWidget* current = widget;
    
    while (current) {
        QString name = current->objectName();
        if (name.isEmpty()) {
            name = current->metaObject()->className();
        }
        path.prepend(name);
        current = current->parentWidget();
    }
    
    return path.join("/");
}

void ContextSensitiveHelpService::trackUserAction(const QString& action)
{
    m_recentActions.append(action);
    
    // Keep only recent actions
    while (m_recentActions.size() > MAX_RECENT_ACTIONS) {
        m_recentActions.removeFirst();
    }
    
    // Update current context
    m_currentContext.recentActions = m_recentActions;
}

bool ContextSensitiveHelpService::hasPrerequisiteKnowledge(const QStringList& prerequisites) const
{
    if (prerequisites.isEmpty()) {
        return true;
    }

    // Check if user has completed prerequisite tutorials
    for (const QString& prerequisite : prerequisites) {
        if (!m_completedTutorials.value(prerequisite, false)) {
            return false;
        }
    }
    
    return true;
}