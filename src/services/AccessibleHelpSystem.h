#ifndef ACCESSIBLEHELPSYSTEM_H
#define ACCESSIBLEHELPSYSTEM_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QStringList>
#include <QTextDocument>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QSplitter>
#include <QLabel>
#include <QKeySequence>
#include <QAccessible>

class AccessibilityManager;
class KeyboardNavigationController;

/**
 * @brief Accessible help and documentation system for XFB
 * 
 * The AccessibleHelpSystem provides comprehensive help content with proper
 * accessibility structure, context-sensitive help delivery, and keyboard
 * shortcut documentation. All help content is designed to work seamlessly
 * with ORCA screen reader and follows accessibility best practices.
 * 
 * Key features:
 * - Structured help content with proper heading hierarchy
 * - Accessible search functionality with keyboard navigation
 * - Context-sensitive help based on current focus
 * - Keyboard shortcut reference with clear organization
 * - Guided tutorials with audio descriptions
 * - Step-by-step accessible instructions
 * 
 * @example
 * @code
 * AccessibleHelpSystem* helpSystem = new AccessibleHelpSystem(accessibilityManager);
 * helpSystem->initialize();
 * helpSystem->showContextHelp(currentWidget);
 * helpSystem->showKeyboardShortcuts();
 * @endcode
 * 
 * @see AccessibilityManager, KeyboardNavigationController
 * @since XFB 2.0
 */
class AccessibleHelpSystem : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Help content categories
     */
    enum class HelpCategory {
        GettingStarted,     ///< Basic introduction and setup
        PlayerControls,     ///< Media player functionality
        DatabaseManagement, ///< Music library and database operations
        PlaylistManagement, ///< Playlist creation and management
        KeyboardShortcuts,  ///< Complete keyboard reference
        Accessibility,      ///< Accessibility features and settings
        Troubleshooting,    ///< Common issues and solutions
        Advanced           ///< Advanced features and configuration
    };

    /**
     * @brief Help content structure
     */
    struct HelpContent {
        QString id;                 ///< Unique content identifier
        QString title;              ///< Content title
        QString content;            ///< Main content (HTML formatted)
        HelpCategory category;      ///< Content category
        QStringList keywords;       ///< Search keywords
        QStringList relatedTopics;  ///< Related help topic IDs
        QString contextWidget;      ///< Widget class name for context help
        int priority;              ///< Display priority (higher = more important)
        
        HelpContent() : category(HelpCategory::GettingStarted), priority(0) {}
    };

    /**
     * @brief Keyboard shortcut documentation entry
     */
    struct ShortcutDocumentation {
        QString action;             ///< Action identifier
        QKeySequence sequence;      ///< Key sequence
        QString description;        ///< Human-readable description
        QString context;            ///< Context where shortcut is active
        QString detailedHelp;       ///< Detailed usage instructions
        HelpCategory category;      ///< Help category
        
        ShortcutDocumentation() : category(HelpCategory::KeyboardShortcuts) {}
    };

    /**
     * @brief Tutorial step information
     */
    struct TutorialStep {
        QString title;              ///< Step title
        QString instruction;        ///< Step instruction
        QString audioDescription;   ///< Audio description for screen readers
        QString targetWidget;       ///< Widget to focus during this step
        QKeySequence keySequence;   ///< Key sequence to demonstrate
        bool waitForUserAction;     ///< Whether to wait for user to complete action
        
        TutorialStep() : waitForUserAction(false) {}
    };

    /**
     * @brief Guided tutorial information
     */
    struct GuidedTutorial {
        QString id;                 ///< Tutorial identifier
        QString title;              ///< Tutorial title
        QString description;        ///< Tutorial description
        QList<TutorialStep> steps;  ///< Tutorial steps
        HelpCategory category;      ///< Tutorial category
        QString prerequisite;       ///< Prerequisite tutorial ID (optional)
        
        GuidedTutorial() : category(HelpCategory::GettingStarted) {}
    };

    explicit AccessibleHelpSystem(AccessibilityManager* accessibilityManager, QObject* parent = nullptr);
    ~AccessibleHelpSystem() override;

    /**
     * @brief Initialize the help system
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the help system
     */
    void shutdown();

    /**
     * @brief Show the main help window
     * @param category Optional category to show initially
     */
    void showHelpWindow(HelpCategory category = HelpCategory::GettingStarted);

    /**
     * @brief Show context-sensitive help for a widget
     * @param widget Widget to show help for
     * @return true if context help was found and displayed
     */
    bool showContextHelp(QWidget* widget);

    /**
     * @brief Show keyboard shortcuts reference
     * @param category Optional category to filter shortcuts
     */
    void showKeyboardShortcuts(HelpCategory category = HelpCategory::KeyboardShortcuts);

    /**
     * @brief Search help content
     * @param query Search query
     * @return List of matching help content IDs
     */
    QStringList searchHelpContent(const QString& query);

    /**
     * @brief Get help content by ID
     * @param contentId Content identifier
     * @return Help content, or default if not found
     */
    HelpContent getHelpContent(const QString& contentId) const;

    /**
     * @brief Register help content
     * @param content Help content to register
     */
    void registerHelpContent(const HelpContent& content);

    /**
     * @brief Register shortcut documentation
     * @param shortcut Shortcut documentation to register
     */
    void registerShortcutDocumentation(const ShortcutDocumentation& shortcut);

    /**
     * @brief Register guided tutorial
     * @param tutorial Tutorial to register
     */
    void registerGuidedTutorial(const GuidedTutorial& tutorial);

    /**
     * @brief Start a guided tutorial
     * @param tutorialId Tutorial identifier
     * @return true if tutorial was started successfully
     */
    bool startGuidedTutorial(const QString& tutorialId);

    /**
     * @brief Get available tutorials for a category
     * @param category Help category
     * @return List of tutorial IDs
     */
    QStringList getAvailableTutorials(HelpCategory category) const;

    /**
     * @brief Check if help is available for a widget
     * @param widget Widget to check
     * @return true if context help is available
     */
    bool hasContextHelp(QWidget* widget) const;

public slots:
    /**
     * @brief Handle help request from keyboard shortcut
     */
    void onHelpRequested();

    /**
     * @brief Handle context help request
     * @param widget Widget requesting context help
     */
    void onContextHelpRequested(QWidget* widget);

    /**
     * @brief Handle search query
     * @param query Search query
     */
    void onSearchRequested(const QString& query);

signals:
    /**
     * @brief Emitted when help content is displayed
     * @param contentId Content identifier
     * @param title Content title
     */
    void helpContentDisplayed(const QString& contentId, const QString& title);

    /**
     * @brief Emitted when tutorial step changes
     * @param tutorialId Tutorial identifier
     * @param stepIndex Current step index
     * @param stepTitle Step title
     */
    void tutorialStepChanged(const QString& tutorialId, int stepIndex, const QString& stepTitle);

    /**
     * @brief Emitted when tutorial completes
     * @param tutorialId Tutorial identifier
     */
    void tutorialCompleted(const QString& tutorialId);

private slots:
    /**
     * @brief Handle help window close
     */
    void onHelpWindowClosed();

    /**
     * @brief Handle search text change
     * @param text New search text
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Handle help content selection
     * @param contentId Selected content ID
     */
    void onHelpContentSelected(const QString& contentId);

    /**
     * @brief Handle tutorial navigation
     */
    void onTutorialNext();
    void onTutorialPrevious();
    void onTutorialCancel();

private:
    /**
     * @brief Create the help window UI
     */
    void createHelpWindow();

    /**
     * @brief Setup help window accessibility
     */
    void setupHelpWindowAccessibility();

    /**
     * @brief Load default help content
     */
    void loadDefaultHelpContent();

    /**
     * @brief Load keyboard shortcut documentation
     */
    void loadKeyboardShortcutDocumentation();

    /**
     * @brief Load guided tutorials
     */
    void loadGuidedTutorials();

    /**
     * @brief Create help content tree
     */
    void createHelpContentTree();

    /**
     * @brief Update search results
     * @param results List of matching content IDs
     */
    void updateSearchResults(const QStringList& results);

    /**
     * @brief Display help content
     * @param contentId Content identifier
     */
    void displayHelpContent(const QString& contentId);

    /**
     * @brief Format keyboard shortcut for display
     * @param sequence Key sequence
     * @return Formatted string
     */
    QString formatKeySequence(const QKeySequence& sequence) const;

    /**
     * @brief Get category display name
     * @param category Help category
     * @return Human-readable category name
     */
    QString getCategoryDisplayName(HelpCategory category) const;

    /**
     * @brief Find context help for widget
     * @param widget Widget to find help for
     * @return Help content ID, or empty string if not found
     */
    QString findContextHelpForWidget(QWidget* widget) const;

    /**
     * @brief Create tutorial window
     */
    void createTutorialWindow();

    /**
     * @brief Setup tutorial window accessibility
     */
    void setupTutorialWindowAccessibility();

    /**
     * @brief Display tutorial step
     * @param tutorial Tutorial information
     * @param stepIndex Step index
     */
    void displayTutorialStep(const GuidedTutorial& tutorial, int stepIndex);

    /**
     * @brief Announce tutorial step
     * @param step Tutorial step
     */
    void announceTutorialStep(const TutorialStep& step);

    // Member variables
    AccessibilityManager* m_accessibilityManager;
    KeyboardNavigationController* m_keyboardController;
    
    // Help content management
    QHash<QString, HelpContent> m_helpContent;
    QHash<HelpCategory, QStringList> m_contentByCategory;
    QHash<QString, QStringList> m_searchIndex;
    
    // Shortcut documentation
    QHash<QString, ShortcutDocumentation> m_shortcutDocs;
    QHash<HelpCategory, QStringList> m_shortcutsByCategory;
    
    // Guided tutorials
    QHash<QString, GuidedTutorial> m_tutorials;
    QHash<HelpCategory, QStringList> m_tutorialsByCategory;
    
    // Context help mapping
    QHash<QString, QString> m_contextHelpMapping; // Widget class name -> content ID
    
    // UI components
    QWidget* m_helpWindow;
    QSplitter* m_mainSplitter;
    QTreeWidget* m_contentTree;
    QTextBrowser* m_contentDisplay;
    QLineEdit* m_searchEdit;
    QPushButton* m_searchButton;
    QLabel* m_statusLabel;
    
    // Tutorial UI components
    QWidget* m_tutorialWindow;
    QLabel* m_tutorialTitle;
    QLabel* m_tutorialInstruction;
    QLabel* m_tutorialProgress;
    QPushButton* m_tutorialPrevious;
    QPushButton* m_tutorialNext;
    QPushButton* m_tutorialCancel;
    
    // Tutorial state
    QString m_currentTutorialId;
    int m_currentTutorialStep;
    bool m_tutorialActive;
    
    // State tracking
    bool m_initialized;
    bool m_helpWindowVisible;
    QString m_currentContentId;
    
    // Configuration constants
    static constexpr const char* HELP_WINDOW_TITLE = "XFB Help and Documentation";
    static constexpr const char* TUTORIAL_WINDOW_TITLE = "XFB Guided Tutorial";
    static constexpr int HELP_WINDOW_WIDTH = 800;
    static constexpr int HELP_WINDOW_HEIGHT = 600;
    static constexpr int TUTORIAL_WINDOW_WIDTH = 600;
    static constexpr int TUTORIAL_WINDOW_HEIGHT = 400;
};

Q_DECLARE_METATYPE(AccessibleHelpSystem::HelpCategory)

#endif // ACCESSIBLEHELPSYSTEM_H