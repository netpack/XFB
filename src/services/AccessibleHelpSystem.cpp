#include "AccessibleHelpSystem.h"
#include "AccessibilityManager.h"
#include "KeyboardNavigationController.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QFont>
#include <QKeyEvent>
#include <QShortcut>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>
#include <QDebug>

AccessibleHelpSystem::AccessibleHelpSystem(AccessibilityManager* accessibilityManager, QObject* parent)
    : QObject(parent)
    , m_accessibilityManager(accessibilityManager)
    , m_keyboardController(nullptr)
    , m_helpWindow(nullptr)
    , m_mainSplitter(nullptr)
    , m_contentTree(nullptr)
    , m_contentDisplay(nullptr)
    , m_searchEdit(nullptr)
    , m_searchButton(nullptr)
    , m_statusLabel(nullptr)
    , m_tutorialWindow(nullptr)
    , m_tutorialTitle(nullptr)
    , m_tutorialInstruction(nullptr)
    , m_tutorialProgress(nullptr)
    , m_tutorialPrevious(nullptr)
    , m_tutorialNext(nullptr)
    , m_tutorialCancel(nullptr)
    , m_currentTutorialStep(0)
    , m_tutorialActive(false)
    , m_initialized(false)
    , m_helpWindowVisible(false)
{
    if (m_accessibilityManager) {
        m_keyboardController = m_accessibilityManager->keyboardNavigationController();
    }
}

AccessibleHelpSystem::~AccessibleHelpSystem()
{
    shutdown();
}

bool AccessibleHelpSystem::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "AccessibleHelpSystem: Initializing accessible help system";

    // Load default help content
    loadDefaultHelpContent();
    
    // Load keyboard shortcut documentation
    loadKeyboardShortcutDocumentation();
    
    // Load guided tutorials
    loadGuidedTutorials();
    
    // Register global help shortcut
    if (m_keyboardController) {
        m_keyboardController->registerKeyboardShortcut(
            "show_help",
            QKeySequence(Qt::Key_F1),
            "Show help and documentation",
            "Global",
            false // Not customizable
        );
        
        m_keyboardController->registerKeyboardShortcut(
            "context_help",
            QKeySequence(Qt::SHIFT | Qt::Key_F1),
            "Show context-sensitive help",
            "Global",
            false // Not customizable
        );
        
        // Connect to shortcut signals
        connect(m_keyboardController, &KeyboardNavigationController::shortcutTriggered,
                this, [this](const QString& action, const QKeySequence&) {
                    if (action == "show_help") {
                        onHelpRequested();
                    } else if (action == "context_help") {
                        QWidget* focusWidget = QApplication::focusWidget();
                        if (focusWidget) {
                            onContextHelpRequested(focusWidget);
                        }
                    }
                });
    }

    m_initialized = true;
    qDebug() << "AccessibleHelpSystem: Initialization completed successfully";
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            "Help system initialized. Press F1 for help, Shift+F1 for context help.",
            AccessibilityManager::Priority::Low
        );
    }
    
    return true;
}

void AccessibleHelpSystem::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "AccessibleHelpSystem: Shutting down help system";

    // Close help window if open
    if (m_helpWindow) {
        m_helpWindow->close();
        delete m_helpWindow;
        m_helpWindow = nullptr;
    }
    
    // Close tutorial window if open
    if (m_tutorialWindow) {
        m_tutorialWindow->close();
        delete m_tutorialWindow;
        m_tutorialWindow = nullptr;
    }
    
    // Clear all data
    m_helpContent.clear();
    m_contentByCategory.clear();
    m_searchIndex.clear();
    m_shortcutDocs.clear();
    m_shortcutsByCategory.clear();
    m_tutorials.clear();
    m_tutorialsByCategory.clear();
    m_contextHelpMapping.clear();
    
    m_initialized = false;
}

void AccessibleHelpSystem::showHelpWindow(HelpCategory category)
{
    if (!m_initialized) {
        qWarning() << "AccessibleHelpSystem: Cannot show help window - system not initialized";
        return;
    }

    if (!m_helpWindow) {
        createHelpWindow();
        setupHelpWindowAccessibility();
    }

    // Show the window
    m_helpWindow->show();
    m_helpWindow->raise();
    m_helpWindow->activateWindow();
    m_helpWindowVisible = true;

    // Select the appropriate category in the tree
    if (m_contentTree) {
        QString categoryName = getCategoryDisplayName(category);
        QTreeWidgetItem* categoryItem = nullptr;
        
        for (int i = 0; i < m_contentTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_contentTree->topLevelItem(i);
            if (item && item->text(0) == categoryName) {
                categoryItem = item;
                break;
            }
        }
        
        if (categoryItem) {
            m_contentTree->setCurrentItem(categoryItem);
            m_contentTree->expandItem(categoryItem);
        }
    }

    // Focus the content tree for keyboard navigation
    if (m_contentTree) {
        m_contentTree->setFocus();
    }

    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            QString("Help window opened. Currently showing %1 category.")
                .arg(getCategoryDisplayName(category)),
            AccessibilityManager::Priority::Normal
        );
    }
}

bool AccessibleHelpSystem::showContextHelp(QWidget* widget)
{
    if (!widget || !m_initialized) {
        return false;
    }

    QString contentId = findContextHelpForWidget(widget);
    if (contentId.isEmpty()) {
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                "No context help available for this control.",
                AccessibilityManager::Priority::Normal
            );
        }
        return false;
    }

    // Show help window with the specific content
    showHelpWindow();
    displayHelpContent(contentId);

    if (m_accessibilityManager) {
        HelpContent content = getHelpContent(contentId);
        m_accessibilityManager->announceMessage(
            QString("Context help: %1").arg(content.title),
            AccessibilityManager::Priority::Normal
        );
    }

    return true;
}

void AccessibleHelpSystem::showKeyboardShortcuts(HelpCategory category)
{
    showHelpWindow(category);
    
    // Find and display keyboard shortcuts content
    QString shortcutsContentId = "keyboard_shortcuts_reference";
    if (m_helpContent.contains(shortcutsContentId)) {
        displayHelpContent(shortcutsContentId);
    }
}

QStringList AccessibleHelpSystem::searchHelpContent(const QString& query)
{
    QStringList results;
    
    if (query.trimmed().isEmpty()) {
        return results;
    }

    QString lowerQuery = query.toLower();
    QStringList queryWords = lowerQuery.split(' ', Qt::SkipEmptyParts);

    // Search through all help content
    for (auto it = m_helpContent.constBegin(); it != m_helpContent.constEnd(); ++it) {
        const HelpContent& content = it.value();
        bool matches = false;
        
        // Check title
        if (content.title.toLower().contains(lowerQuery)) {
            matches = true;
        }
        
        // Check keywords
        if (!matches) {
            for (const QString& keyword : content.keywords) {
                if (keyword.toLower().contains(lowerQuery)) {
                    matches = true;
                    break;
                }
            }
        }
        
        // Check content text (basic search)
        if (!matches) {
            QString plainContent = QTextDocumentFragment::fromHtml(content.content).toPlainText();
            if (plainContent.toLower().contains(lowerQuery)) {
                matches = true;
            }
        }
        
        // Check for multiple word matches
        if (!matches && queryWords.size() > 1) {
            int wordMatches = 0;
            QString searchText = (content.title + " " + content.keywords.join(" ")).toLower();
            
            for (const QString& word : queryWords) {
                if (searchText.contains(word)) {
                    wordMatches++;
                }
            }
            
            if (wordMatches >= queryWords.size() / 2) { // At least half the words match
                matches = true;
            }
        }
        
        if (matches) {
            results.append(it.key());
        }
    }
    
    // Sort results by priority
    std::sort(results.begin(), results.end(), [this](const QString& a, const QString& b) {
        return m_helpContent[a].priority > m_helpContent[b].priority;
    });

    return results;
}

AccessibleHelpSystem::HelpContent AccessibleHelpSystem::getHelpContent(const QString& contentId) const
{
    return m_helpContent.value(contentId, HelpContent());
}

void AccessibleHelpSystem::registerHelpContent(const HelpContent& content)
{
    if (content.id.isEmpty()) {
        qWarning() << "AccessibleHelpSystem: Cannot register help content with empty ID";
        return;
    }

    m_helpContent[content.id] = content;
    
    // Add to category index
    if (!m_contentByCategory[content.category].contains(content.id)) {
        m_contentByCategory[content.category].append(content.id);
    }
    
    // Add to search index
    QStringList searchTerms = content.keywords;
    searchTerms.append(content.title.split(' ', Qt::SkipEmptyParts));
    
    for (const QString& term : searchTerms) {
        QString lowerTerm = term.toLower();
        if (!m_searchIndex[lowerTerm].contains(content.id)) {
            m_searchIndex[lowerTerm].append(content.id);
        }
    }
    
    // Add context mapping if specified
    if (!content.contextWidget.isEmpty()) {
        m_contextHelpMapping[content.contextWidget] = content.id;
    }
}

void AccessibleHelpSystem::registerShortcutDocumentation(const ShortcutDocumentation& shortcut)
{
    if (shortcut.action.isEmpty()) {
        qWarning() << "AccessibleHelpSystem: Cannot register shortcut documentation with empty action";
        return;
    }

    m_shortcutDocs[shortcut.action] = shortcut;
    
    // Add to category index
    if (!m_shortcutsByCategory[shortcut.category].contains(shortcut.action)) {
        m_shortcutsByCategory[shortcut.category].append(shortcut.action);
    }
}

void AccessibleHelpSystem::registerGuidedTutorial(const GuidedTutorial& tutorial)
{
    if (tutorial.id.isEmpty()) {
        qWarning() << "AccessibleHelpSystem: Cannot register tutorial with empty ID";
        return;
    }

    m_tutorials[tutorial.id] = tutorial;
    
    // Add to category index
    if (!m_tutorialsByCategory[tutorial.category].contains(tutorial.id)) {
        m_tutorialsByCategory[tutorial.category].append(tutorial.id);
    }
}

bool AccessibleHelpSystem::startGuidedTutorial(const QString& tutorialId)
{
    if (!m_tutorials.contains(tutorialId)) {
        qWarning() << "AccessibleHelpSystem: Tutorial not found:" << tutorialId;
        return false;
    }

    const GuidedTutorial& tutorial = m_tutorials[tutorialId];
    
    if (tutorial.steps.isEmpty()) {
        qWarning() << "AccessibleHelpSystem: Tutorial has no steps:" << tutorialId;
        return false;
    }

    // Create tutorial window if needed
    if (!m_tutorialWindow) {
        createTutorialWindow();
        setupTutorialWindowAccessibility();
    }

    m_currentTutorialId = tutorialId;
    m_currentTutorialStep = 0;
    m_tutorialActive = true;

    // Show tutorial window
    m_tutorialWindow->show();
    m_tutorialWindow->raise();
    m_tutorialWindow->activateWindow();

    // Display first step
    displayTutorialStep(tutorial, 0);

    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            QString("Started tutorial: %1. %2 steps total.")
                .arg(tutorial.title)
                .arg(tutorial.steps.size()),
            AccessibilityManager::Priority::Normal
        );
    }

    emit tutorialStepChanged(tutorialId, 0, tutorial.steps[0].title);
    return true;
}

QStringList AccessibleHelpSystem::getAvailableTutorials(HelpCategory category) const
{
    return m_tutorialsByCategory.value(category, QStringList());
}

bool AccessibleHelpSystem::hasContextHelp(QWidget* widget) const
{
    return !findContextHelpForWidget(widget).isEmpty();
}

void AccessibleHelpSystem::onHelpRequested()
{
    showHelpWindow();
}

void AccessibleHelpSystem::onContextHelpRequested(QWidget* widget)
{
    showContextHelp(widget);
}

void AccessibleHelpSystem::onSearchRequested(const QString& query)
{
    QStringList results = searchHelpContent(query);
    updateSearchResults(results);
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            QString("Search completed. %1 results found.").arg(results.size()),
            AccessibilityManager::Priority::Normal
        );
    }
}

void AccessibleHelpSystem::onHelpWindowClosed()
{
    m_helpWindowVisible = false;
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(
            "Help window closed.",
            AccessibilityManager::Priority::Low
        );
    }
}

void AccessibleHelpSystem::onSearchTextChanged(const QString& text)
{
    if (text.length() >= 3) { // Start searching after 3 characters
        QTimer::singleShot(300, this, [this, text]() {
            if (m_searchEdit && m_searchEdit->text() == text) {
                onSearchRequested(text);
            }
        });
    } else if (text.isEmpty()) {
        // Clear search results
        updateSearchResults(QStringList());
    }
}

void AccessibleHelpSystem::onHelpContentSelected(const QString& contentId)
{
    displayHelpContent(contentId);
}

void AccessibleHelpSystem::onTutorialNext()
{
    if (!m_tutorialActive || m_currentTutorialId.isEmpty()) {
        return;
    }

    const GuidedTutorial& tutorial = m_tutorials[m_currentTutorialId];
    
    if (m_currentTutorialStep < tutorial.steps.size() - 1) {
        m_currentTutorialStep++;
        displayTutorialStep(tutorial, m_currentTutorialStep);
        emit tutorialStepChanged(m_currentTutorialId, m_currentTutorialStep, 
                                tutorial.steps[m_currentTutorialStep].title);
    } else {
        // Tutorial completed
        m_tutorialActive = false;
        
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                QString("Tutorial completed: %1").arg(tutorial.title),
                AccessibilityManager::Priority::Normal
            );
        }
        
        emit tutorialCompleted(m_currentTutorialId);
        
        if (m_tutorialWindow) {
            m_tutorialWindow->close();
        }
    }
}

void AccessibleHelpSystem::onTutorialPrevious()
{
    if (!m_tutorialActive || m_currentTutorialId.isEmpty()) {
        return;
    }

    if (m_currentTutorialStep > 0) {
        m_currentTutorialStep--;
        const GuidedTutorial& tutorial = m_tutorials[m_currentTutorialId];
        displayTutorialStep(tutorial, m_currentTutorialStep);
        emit tutorialStepChanged(m_currentTutorialId, m_currentTutorialStep, 
                                tutorial.steps[m_currentTutorialStep].title);
    }
}

void AccessibleHelpSystem::onTutorialCancel()
{
    if (m_tutorialActive) {
        m_tutorialActive = false;
        
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage(
                "Tutorial cancelled.",
                AccessibilityManager::Priority::Normal
            );
        }
        
        if (m_tutorialWindow) {
            m_tutorialWindow->close();
        }
    }
}

QString AccessibleHelpSystem::getCategoryDisplayName(HelpCategory category) const
{
    switch (category) {
        case HelpCategory::GettingStarted:
            return "Getting Started";
        case HelpCategory::PlayerControls:
            return "Player Controls";
        case HelpCategory::DatabaseManagement:
            return "Database Management";
        case HelpCategory::PlaylistManagement:
            return "Playlist Management";
        case HelpCategory::KeyboardShortcuts:
            return "Keyboard Shortcuts";
        case HelpCategory::Accessibility:
            return "Accessibility Features";
        case HelpCategory::Troubleshooting:
            return "Troubleshooting";
        case HelpCategory::Advanced:
            return "Advanced Features";
        default:
            return "General";
    }
}

QString AccessibleHelpSystem::findContextHelpForWidget(QWidget* widget) const
{
    if (!widget) {
        return QString();
    }

    // Check direct mapping by class name
    QString className = widget->metaObject()->className();
    if (m_contextHelpMapping.contains(className)) {
        return m_contextHelpMapping[className];
    }
    
    // Check parent widgets
    QWidget* parent = widget->parentWidget();
    while (parent) {
        QString parentClassName = parent->metaObject()->className();
        if (m_contextHelpMapping.contains(parentClassName)) {
            return m_contextHelpMapping[parentClassName];
        }
        parent = parent->parentWidget();
    }
    
    // Check by object name
    QString objectName = widget->objectName();
    if (!objectName.isEmpty() && m_contextHelpMapping.contains(objectName)) {
        return m_contextHelpMapping[objectName];
    }
    
    return QString();
}

// Implementation continues in next part due to length...vo
id AccessibleHelpSystem::createHelpWindow()
{
    m_helpWindow = new QWidget();
    m_helpWindow->setWindowTitle(HELP_WINDOW_TITLE);
    m_helpWindow->resize(HELP_WINDOW_WIDTH, HELP_WINDOW_HEIGHT);
    m_helpWindow->setAttribute(Qt::WA_DeleteOnClose, false);

    // Center the window
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - HELP_WINDOW_WIDTH) / 2;
    int y = (screenGeometry.height() - HELP_WINDOW_HEIGHT) / 2;
    m_helpWindow->move(x, y);

    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(m_helpWindow);

    // Create search section
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    QLabel* searchLabel = new QLabel("Search help:");
    searchLabel->setObjectName("searchLabel");
    searchLayout->addWidget(searchLabel);
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Enter search terms...");
    searchLabel->setBuddy(m_searchEdit);
    searchLayout->addWidget(m_searchEdit);
    
    m_searchButton = new QPushButton("Search");
    m_searchButton->setObjectName("searchButton");
    searchLayout->addWidget(m_searchButton);
    
    mainLayout->addLayout(searchLayout);

    // Create main content area
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setObjectName("mainSplitter");

    // Create content tree
    m_contentTree = new QTreeWidget();
    m_contentTree->setObjectName("contentTree");
    m_contentTree->setHeaderLabel("Help Topics");
    m_contentTree->setMinimumWidth(250);
    m_mainSplitter->addWidget(m_contentTree);

    // Create content display
    m_contentDisplay = new QTextBrowser();
    m_contentDisplay->setObjectName("contentDisplay");
    m_contentDisplay->setMinimumWidth(400);
    m_contentDisplay->setOpenExternalLinks(false);
    m_mainSplitter->addWidget(m_contentDisplay);

    // Set splitter proportions
    m_mainSplitter->setSizes({250, 550});
    mainLayout->addWidget(m_mainSplitter);

    // Create status bar
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(m_statusLabel);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &AccessibleHelpSystem::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked,
            this, [this]() { onSearchRequested(m_searchEdit->text()); });
    connect(m_contentTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                if (current && current->data(0, Qt::UserRole).isValid()) {
                    QString contentId = current->data(0, Qt::UserRole).toString();
                    onHelpContentSelected(contentId);
                }
            });
    connect(m_helpWindow, &QWidget::destroyed,
            this, &AccessibleHelpSystem::onHelpWindowClosed);

    // Create help content tree
    createHelpContentTree();
}

void AccessibleHelpSystem::setupHelpWindowAccessibility()
{
    if (!m_helpWindow || !m_accessibilityManager) {
        return;
    }

    // Set up accessibility for search components
    if (m_searchEdit) {
        AccessibilityManager::AccessibilityMetadata searchMetadata;
        searchMetadata.name = "Search help content";
        searchMetadata.description = "Enter search terms to find help topics";
        searchMetadata.role = QAccessible::EditableText;
        m_accessibilityManager->registerAccessibleWidget(m_searchEdit, searchMetadata);
    }

    if (m_searchButton) {
        AccessibilityManager::AccessibilityMetadata buttonMetadata;
        buttonMetadata.name = "Search";
        buttonMetadata.description = "Execute search for help content";
        buttonMetadata.role = QAccessible::PushButton;
        m_accessibilityManager->registerAccessibleWidget(m_searchButton, buttonMetadata);
    }

    // Set up accessibility for content tree
    if (m_contentTree) {
        AccessibilityManager::AccessibilityMetadata treeMetadata;
        treeMetadata.name = "Help topics";
        treeMetadata.description = "Navigate help topics by category. Use arrow keys to browse, Enter to select.";
        treeMetadata.role = QAccessible::Tree;
        m_accessibilityManager->registerAccessibleWidget(m_contentTree, treeMetadata);
        
        // Register for keyboard navigation
        if (m_keyboardController) {
            m_keyboardController->registerNavigationWidget(
                m_contentTree,
                KeyboardNavigationController::NavigationContext::List,
                "Help topics tree",
                "Use arrow keys to navigate, Enter to select topic"
            );
        }
    }

    // Set up accessibility for content display
    if (m_contentDisplay) {
        AccessibilityManager::AccessibilityMetadata displayMetadata;
        displayMetadata.name = "Help content";
        displayMetadata.description = "Help content display area. Use arrow keys to scroll, Tab to navigate links.";
        displayMetadata.role = QAccessible::Document;
        m_accessibilityManager->registerAccessibleWidget(m_contentDisplay, displayMetadata);
    }

    // Set up accessibility for status label
    if (m_statusLabel) {
        AccessibilityManager::AccessibilityMetadata statusMetadata;
        statusMetadata.name = "Status";
        statusMetadata.description = "Current help system status";
        statusMetadata.role = QAccessible::StaticText;
        statusMetadata.isLiveRegion = true;
        m_accessibilityManager->registerAccessibleWidget(m_statusLabel, statusMetadata);
    }
}

void AccessibleHelpSystem::loadDefaultHelpContent()
{
    // Getting Started content
    HelpContent gettingStarted;
    gettingStarted.id = "getting_started_overview";
    gettingStarted.title = "Getting Started with XFB";
    gettingStarted.category = HelpCategory::GettingStarted;
    gettingStarted.priority = 100;
    gettingStarted.keywords = {"introduction", "overview", "basics", "start", "begin"};
    gettingStarted.content = R"(
        <h1>Welcome to XFB Radio Broadcasting Software</h1>
        <p>XFB is a professional radio automation software designed with full accessibility support for visually impaired broadcasters.</p>
        
        <h2>Key Features</h2>
        <ul>
            <li>Complete keyboard navigation support</li>
            <li>ORCA screen reader integration</li>
            <li>Audio feedback for all operations</li>
            <li>Accessible database management</li>
            <li>Professional broadcasting tools</li>
        </ul>
        
        <h2>Getting Help</h2>
        <p>Press <strong>F1</strong> at any time to open this help system.</p>
        <p>Press <strong>Shift+F1</strong> for context-sensitive help on the current control.</p>
        
        <h2>First Steps</h2>
        <ol>
            <li>Configure your accessibility preferences</li>
            <li>Set up your music library</li>
            <li>Learn the keyboard shortcuts</li>
            <li>Try the guided tutorials</li>
        </ol>
    )";
    registerHelpContent(gettingStarted);

    // Player Controls content
    HelpContent playerControls;
    playerControls.id = "player_controls_overview";
    playerControls.title = "Player Controls";
    playerControls.category = HelpCategory::PlayerControls;
    playerControls.priority = 90;
    playerControls.keywords = {"player", "playback", "controls", "play", "pause", "stop"};
    playerControls.contextWidget = "player";
    playerControls.content = R"(
        <h1>Player Controls</h1>
        <p>The XFB player provides full keyboard control for all playback operations.</p>
        
        <h2>Basic Playback Controls</h2>
        <ul>
            <li><strong>Space</strong> - Play/Pause toggle</li>
            <li><strong>Ctrl+S</strong> - Stop playback</li>
            <li><strong>Ctrl+Right</strong> - Next track</li>
            <li><strong>Ctrl+Left</strong> - Previous track</li>
        </ul>
        
        <h2>Volume and Seeking</h2>
        <ul>
            <li><strong>Ctrl+Up</strong> - Increase volume</li>
            <li><strong>Ctrl+Down</strong> - Decrease volume</li>
            <li><strong>Shift+Right</strong> - Seek forward 10 seconds</li>
            <li><strong>Shift+Left</strong> - Seek backward 10 seconds</li>
        </ul>
        
        <h2>Audio Feedback</h2>
        <p>All player operations provide immediate audio feedback through ORCA announcements.</p>
        <p>Press <strong>Ctrl+T</strong> to hear the current playback time without interrupting other announcements.</p>
    )";
    registerHelpContent(playerControls);

    // Database Management content
    HelpContent databaseManagement;
    databaseManagement.id = "database_management_overview";
    databaseManagement.title = "Database Management";
    databaseManagement.category = HelpCategory::DatabaseManagement;
    databaseManagement.priority = 85;
    databaseManagement.keywords = {"database", "library", "music", "tracks", "management"};
    databaseManagement.contextWidget = "QTableView";
    databaseManagement.content = R"(
        <h1>Database Management</h1>
        <p>XFB provides accessible tools for managing your music library and database.</p>
        
        <h2>Grid Navigation</h2>
        <ul>
            <li><strong>Arrow keys</strong> - Navigate between cells</li>
            <li><strong>Home/End</strong> - Move to first/last column</li>
            <li><strong>Ctrl+Home/End</strong> - Move to first/last row</li>
            <li><strong>Page Up/Down</strong> - Navigate by pages</li>
        </ul>
        
        <h2>Editing Operations</h2>
        <ul>
            <li><strong>F2</strong> - Enter edit mode for current cell</li>
            <li><strong>Enter</strong> - Save changes and move to next row</li>
            <li><strong>Escape</strong> - Cancel editing</li>
            <li><strong>Tab</strong> - Move to next editable cell</li>
        </ul>
        
        <h2>Selection and Operations</h2>
        <ul>
            <li><strong>Ctrl+A</strong> - Select all items</li>
            <li><strong>Ctrl+C</strong> - Copy selected items</li>
            <li><strong>Ctrl+X</strong> - Cut selected items</li>
            <li><strong>Ctrl+V</strong> - Paste items</li>
            <li><strong>Delete</strong> - Remove selected items</li>
        </ul>
    )";
    registerHelpContent(databaseManagement);

    // Keyboard Shortcuts Reference
    HelpContent keyboardShortcuts;
    keyboardShortcuts.id = "keyboard_shortcuts_reference";
    keyboardShortcuts.title = "Keyboard Shortcuts Reference";
    keyboardShortcuts.category = HelpCategory::KeyboardShortcuts;
    keyboardShortcuts.priority = 95;
    keyboardShortcuts.keywords = {"shortcuts", "keyboard", "keys", "hotkeys", "commands"};
    keyboardShortcuts.content = R"(
        <h1>Keyboard Shortcuts Reference</h1>
        <p>Complete reference of all keyboard shortcuts available in XFB.</p>
        
        <h2>Global Shortcuts</h2>
        <ul>
            <li><strong>F1</strong> - Show help</li>
            <li><strong>Shift+F1</strong> - Context-sensitive help</li>
            <li><strong>Ctrl+Q</strong> - Quit application</li>
            <li><strong>Ctrl+,</strong> - Open preferences</li>
        </ul>
        
        <h2>Player Shortcuts</h2>
        <ul>
            <li><strong>Space</strong> - Play/Pause</li>
            <li><strong>Ctrl+S</strong> - Stop</li>
            <li><strong>Ctrl+T</strong> - Announce current time</li>
            <li><strong>Ctrl+Right/Left</strong> - Next/Previous track</li>
            <li><strong>Ctrl+Up/Down</strong> - Volume up/down</li>
        </ul>
        
        <h2>Database Shortcuts</h2>
        <ul>
            <li><strong>Ctrl+F</strong> - Find/Search</li>
            <li><strong>F3</strong> - Find next</li>
            <li><strong>F5</strong> - Refresh</li>
            <li><strong>Ctrl+N</strong> - New item</li>
            <li><strong>F2</strong> - Edit current item</li>
        </ul>
        
        <p><em>Note: All shortcuts can be customized in the accessibility preferences.</em></p>
    )";
    registerHelpContent(keyboardShortcuts);

    // Accessibility Features
    HelpContent accessibilityFeatures;
    accessibilityFeatures.id = "accessibility_features_overview";
    accessibilityFeatures.title = "Accessibility Features";
    accessibilityFeatures.category = HelpCategory::Accessibility;
    accessibilityFeatures.priority = 88;
    accessibilityFeatures.keywords = {"accessibility", "orca", "screen reader", "braille", "settings"};
    accessibilityFeatures.contextWidget = "AccessibilityPreferencesDialog";
    accessibilityFeatures.content = R"(
        <h1>Accessibility Features</h1>
        <p>XFB provides comprehensive accessibility support for visually impaired users.</p>
        
        <h2>Screen Reader Support</h2>
        <ul>
            <li>Full ORCA integration with AT-SPI</li>
            <li>Proper accessible names and descriptions</li>
            <li>Live region announcements for dynamic content</li>
            <li>Context-sensitive help and navigation hints</li>
        </ul>
        
        <h2>Audio Feedback</h2>
        <ul>
            <li>Immediate confirmation for all user actions</li>
            <li>Customizable verbosity levels (Terse, Normal, Verbose)</li>
            <li>Priority-based announcement queuing</li>
            <li>Background operation progress announcements</li>
        </ul>
        
        <h2>Keyboard Navigation</h2>
        <ul>
            <li>Logical tab order following visual flow</li>
            <li>Arrow key navigation in grids and lists</li>
            <li>Customizable keyboard shortcuts</li>
            <li>Focus management with clear announcements</li>
        </ul>
        
        <h2>Braille Support</h2>
        <ul>
            <li>Braille display integration</li>
            <li>Customizable braille formatting</li>
            <li>Optimized braille output for efficiency</li>
        </ul>
        
        <h2>Customization</h2>
        <p>Access accessibility preferences through the main menu or press <strong>Ctrl+Alt+A</strong>.</p>
    )";
    registerHelpContent(accessibilityFeatures);
}

void AccessibleHelpSystem::loadKeyboardShortcutDocumentation()
{
    if (!m_keyboardController) {
        return;
    }

    // Get all registered shortcuts from the keyboard controller
    QHash<QString, KeyboardNavigationController::KeyboardShortcut> shortcuts = 
        m_keyboardController->getKeyboardShortcuts();

    for (auto it = shortcuts.constBegin(); it != shortcuts.constEnd(); ++it) {
        const auto& shortcut = it.value();
        
        ShortcutDocumentation doc;
        doc.action = shortcut.action;
        doc.sequence = shortcut.sequence;
        doc.description = shortcut.description;
        doc.context = shortcut.context;
        
        // Determine category based on context
        if (shortcut.context.contains("Player", Qt::CaseInsensitive)) {
            doc.category = HelpCategory::PlayerControls;
        } else if (shortcut.context.contains("Database", Qt::CaseInsensitive)) {
            doc.category = HelpCategory::DatabaseManagement;
        } else if (shortcut.context.contains("Playlist", Qt::CaseInsensitive)) {
            doc.category = HelpCategory::PlaylistManagement;
        } else {
            doc.category = HelpCategory::KeyboardShortcuts;
        }
        
        // Add detailed help based on action
        if (shortcut.action == "play_pause") {
            doc.detailedHelp = "Toggles between play and pause states. When no track is loaded, starts playback of the first track in the current playlist.";
        } else if (shortcut.action == "show_help") {
            doc.detailedHelp = "Opens the main help window with full documentation and tutorials.";
        } else if (shortcut.action == "context_help") {
            doc.detailedHelp = "Shows context-sensitive help for the currently focused control or window.";
        }
        
        registerShortcutDocumentation(doc);
    }
}

void AccessibleHelpSystem::loadGuidedTutorials()
{
    // Basic Navigation Tutorial
    GuidedTutorial basicNavigation;
    basicNavigation.id = "basic_navigation_tutorial";
    basicNavigation.title = "Basic Navigation Tutorial";
    basicNavigation.description = "Learn the fundamentals of navigating XFB with keyboard and screen reader.";
    basicNavigation.category = HelpCategory::GettingStarted;
    
    TutorialStep step1;
    step1.title = "Welcome to XFB";
    step1.instruction = "Welcome to the XFB basic navigation tutorial. This tutorial will teach you how to navigate the interface using keyboard and screen reader.";
    step1.audioDescription = "Tutorial started. You will learn keyboard navigation basics.";
    step1.waitForUserAction = false;
    basicNavigation.steps.append(step1);
    
    TutorialStep step2;
    step2.title = "Tab Navigation";
    step2.instruction = "Press Tab to move between controls. Press Shift+Tab to move backwards. Try navigating through the main interface now.";
    step2.audioDescription = "Practice using Tab and Shift+Tab to navigate between controls.";
    step2.keySequence = QKeySequence(Qt::Key_Tab);
    step2.waitForUserAction = true;
    basicNavigation.steps.append(step2);
    
    TutorialStep step3;
    step3.title = "Getting Help";
    step3.instruction = "Press F1 at any time to open help. Press Shift+F1 for context-sensitive help on the current control.";
    step3.audioDescription = "Remember: F1 for general help, Shift+F1 for context help.";
    step3.keySequence = QKeySequence(Qt::Key_F1);
    step3.waitForUserAction = false;
    basicNavigation.steps.append(step3);
    
    registerGuidedTutorial(basicNavigation);

    // Player Controls Tutorial
    GuidedTutorial playerTutorial;
    playerTutorial.id = "player_controls_tutorial";
    playerTutorial.title = "Player Controls Tutorial";
    playerTutorial.description = "Learn how to control audio playback using keyboard shortcuts.";
    playerTutorial.category = HelpCategory::PlayerControls;
    playerTutorial.prerequisite = "basic_navigation_tutorial";
    
    TutorialStep playerStep1;
    playerStep1.title = "Basic Playback";
    playerStep1.instruction = "Press Space to play or pause audio. The player will announce the current state.";
    playerStep1.audioDescription = "Space bar controls play and pause. Listen for the announcement.";
    playerStep1.keySequence = QKeySequence(Qt::Key_Space);
    playerStep1.waitForUserAction = true;
    playerTutorial.steps.append(playerStep1);
    
    TutorialStep playerStep2;
    playerStep2.title = "Volume Control";
    playerStep2.instruction = "Use Ctrl+Up and Ctrl+Down to adjust volume. Try changing the volume now.";
    playerStep2.audioDescription = "Control volume with Ctrl+Up and Ctrl+Down arrow keys.";
    playerStep2.keySequence = QKeySequence(Qt::CTRL | Qt::Key_Up);
    playerStep2.waitForUserAction = true;
    playerTutorial.steps.append(playerStep2);
    
    TutorialStep playerStep3;
    playerStep3.title = "Track Navigation";
    playerStep3.instruction = "Use Ctrl+Right and Ctrl+Left to skip between tracks in your playlist.";
    playerStep3.audioDescription = "Navigate tracks with Ctrl+Right and Ctrl+Left arrow keys.";
    playerStep3.keySequence = QKeySequence(Qt::CTRL | Qt::Key_Right);
    playerStep3.waitForUserAction = false;
    playerTutorial.steps.append(playerStep3);
    
    registerGuidedTutorial(playerTutorial);
}

void AccessibleHelpSystem::createHelpContentTree()
{
    if (!m_contentTree) {
        return;
    }

    m_contentTree->clear();

    // Create category items
    QHash<HelpCategory, QTreeWidgetItem*> categoryItems;
    
    for (auto it = m_contentByCategory.constBegin(); it != m_contentByCategory.constEnd(); ++it) {
        HelpCategory category = it.key();
        const QStringList& contentIds = it.value();
        
        if (contentIds.isEmpty()) {
            continue;
        }
        
        QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_contentTree);
        categoryItem->setText(0, getCategoryDisplayName(category));
        categoryItem->setExpanded(false);
        categoryItems[category] = categoryItem;
        
        // Add content items
        for (const QString& contentId : contentIds) {
            const HelpContent& content = m_helpContent[contentId];
            
            QTreeWidgetItem* contentItem = new QTreeWidgetItem(categoryItem);
            contentItem->setText(0, content.title);
            contentItem->setData(0, Qt::UserRole, contentId);
        }
    }
    
    // Expand the Getting Started category by default
    if (categoryItems.contains(HelpCategory::GettingStarted)) {
        categoryItems[HelpCategory::GettingStarted]->setExpanded(true);
    }
}

void AccessibleHelpSystem::updateSearchResults(const QStringList& results)
{
    if (!m_contentTree) {
        return;
    }

    if (results.isEmpty()) {
        // Restore full tree
        createHelpContentTree();
        if (m_statusLabel) {
            m_statusLabel->setText("Ready");
        }
        return;
    }

    // Clear tree and show only search results
    m_contentTree->clear();
    
    QTreeWidgetItem* resultsItem = new QTreeWidgetItem(m_contentTree);
    resultsItem->setText(0, QString("Search Results (%1)").arg(results.size()));
    resultsItem->setExpanded(true);
    
    for (const QString& contentId : results) {
        const HelpContent& content = m_helpContent[contentId];
        
        QTreeWidgetItem* resultItem = new QTreeWidgetItem(resultsItem);
        resultItem->setText(0, content.title);
        resultItem->setData(0, Qt::UserRole, contentId);
    }
    
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Found %1 results").arg(results.size()));
    }
}

void AccessibleHelpSystem::displayHelpContent(const QString& contentId)
{
    if (!m_contentDisplay || contentId.isEmpty()) {
        return;
    }

    const HelpContent& content = getHelpContent(contentId);
    if (content.id.isEmpty()) {
        m_contentDisplay->setHtml("<h1>Content Not Found</h1><p>The requested help content could not be found.</p>");
        return;
    }

    m_contentDisplay->setHtml(content.content);
    m_currentContentId = contentId;
    
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Displaying: %1").arg(content.title));
    }
    
    emit helpContentDisplayed(contentId, content.title);
}

QString AccessibleHelpSystem::formatKeySequence(const QKeySequence& sequence) const
{
    return sequence.toString(QKeySequence::NativeText);
}

void AccessibleHelpSystem::createTutorialWindow()
{
    m_tutorialWindow = new QWidget();
    m_tutorialWindow->setWindowTitle(TUTORIAL_WINDOW_TITLE);
    m_tutorialWindow->resize(TUTORIAL_WINDOW_WIDTH, TUTORIAL_WINDOW_HEIGHT);
    m_tutorialWindow->setAttribute(Qt::WA_DeleteOnClose, false);

    // Center the window
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - TUTORIAL_WINDOW_WIDTH) / 2;
    int y = (screenGeometry.height() - TUTORIAL_WINDOW_HEIGHT) / 2;
    m_tutorialWindow->move(x, y);

    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(m_tutorialWindow);

    // Tutorial title
    m_tutorialTitle = new QLabel();
    m_tutorialTitle->setObjectName("tutorialTitle");
    m_tutorialTitle->setStyleSheet("font-weight: bold; font-size: 14pt;");
    layout->addWidget(m_tutorialTitle);

    // Tutorial instruction
    m_tutorialInstruction = new QLabel();
    m_tutorialInstruction->setObjectName("tutorialInstruction");
    m_tutorialInstruction->setWordWrap(true);
    m_tutorialInstruction->setMinimumHeight(100);
    layout->addWidget(m_tutorialInstruction);

    // Progress indicator
    m_tutorialProgress = new QLabel();
    m_tutorialProgress->setObjectName("tutorialProgress");
    layout->addWidget(m_tutorialProgress);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_tutorialPrevious = new QPushButton("Previous");
    m_tutorialPrevious->setObjectName("tutorialPrevious");
    buttonLayout->addWidget(m_tutorialPrevious);
    
    buttonLayout->addStretch();
    
    m_tutorialCancel = new QPushButton("Cancel");
    m_tutorialCancel->setObjectName("tutorialCancel");
    buttonLayout->addWidget(m_tutorialCancel);
    
    m_tutorialNext = new QPushButton("Next");
    m_tutorialNext->setObjectName("tutorialNext");
    buttonLayout->addWidget(m_tutorialNext);
    
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(m_tutorialPrevious, &QPushButton::clicked,
            this, &AccessibleHelpSystem::onTutorialPrevious);
    connect(m_tutorialNext, &QPushButton::clicked,
            this, &AccessibleHelpSystem::onTutorialNext);
    connect(m_tutorialCancel, &QPushButton::clicked,
            this, &AccessibleHelpSystem::onTutorialCancel);
}

void AccessibleHelpSystem::setupTutorialWindowAccessibility()
{
    if (!m_tutorialWindow || !m_accessibilityManager) {
        return;
    }

    // Set up accessibility for tutorial components
    if (m_tutorialTitle) {
        AccessibilityManager::AccessibilityMetadata titleMetadata;
        titleMetadata.name = "Tutorial title";
        titleMetadata.role = QAccessible::Heading;
        m_accessibilityManager->registerAccessibleWidget(m_tutorialTitle, titleMetadata);
    }

    if (m_tutorialInstruction) {
        AccessibilityManager::AccessibilityMetadata instructionMetadata;
        instructionMetadata.name = "Tutorial instruction";
        instructionMetadata.role = QAccessible::StaticText;
        instructionMetadata.isLiveRegion = true;
        m_accessibilityManager->registerAccessibleWidget(m_tutorialInstruction, instructionMetadata);
    }

    if (m_tutorialProgress) {
        AccessibilityManager::AccessibilityMetadata progressMetadata;
        progressMetadata.name = "Tutorial progress";
        progressMetadata.role = QAccessible::StaticText;
        progressMetadata.isLiveRegion = true;
        m_accessibilityManager->registerAccessibleWidget(m_tutorialProgress, progressMetadata);
    }
}

void AccessibleHelpSystem::displayTutorialStep(const GuidedTutorial& tutorial, int stepIndex)
{
    if (stepIndex < 0 || stepIndex >= tutorial.steps.size()) {
        return;
    }

    const TutorialStep& step = tutorial.steps[stepIndex];

    if (m_tutorialTitle) {
        m_tutorialTitle->setText(tutorial.title);
    }

    if (m_tutorialInstruction) {
        m_tutorialInstruction->setText(step.instruction);
    }

    if (m_tutorialProgress) {
        m_tutorialProgress->setText(QString("Step %1 of %2: %3")
                                   .arg(stepIndex + 1)
                                   .arg(tutorial.steps.size())
                                   .arg(step.title));
    }

    // Update button states
    if (m_tutorialPrevious) {
        m_tutorialPrevious->setEnabled(stepIndex > 0);
    }

    if (m_tutorialNext) {
        bool isLastStep = (stepIndex == tutorial.steps.size() - 1);
        m_tutorialNext->setText(isLastStep ? "Finish" : "Next");
    }

    // Announce the step
    announceTutorialStep(step);
}

void AccessibleHelpSystem::announceTutorialStep(const TutorialStep& step)
{
    if (!m_accessibilityManager) {
        return;
    }

    QString announcement = step.audioDescription.isEmpty() ? step.instruction : step.audioDescription;
    
    if (!step.keySequence.isEmpty()) {
        announcement += QString(" Key sequence: %1").arg(formatKeySequence(step.keySequence));
    }

    m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::High);
}