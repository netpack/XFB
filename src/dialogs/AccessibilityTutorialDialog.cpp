#include "AccessibilityTutorialDialog.h"

#include <QListWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeySequence>
#include <QShortcut>
#include <QTextCursor>

namespace {

// Render a shortcut the way the current platform writes it, so the tutorial
// says "Cmd+Shift+P" on macOS and "Ctrl+Shift+P" elsewhere without keeping a
// second, hand-written copy of the key table in prose.
// Takes QKeyCombination rather than int: the implicit conversion to int has
// been deprecated since Qt 6.0 and relying on it breaks on stricter builds.
QString key(QKeyCombination combination)
{
    return QKeySequence(combination).toString(QKeySequence::NativeText).toHtmlEscaped();
}

} // namespace

AccessibilityTutorialDialog::AccessibilityTutorialDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("XFB Tutorial for Screen Reader Users"));
    setAccessibleName(tr("XFB tutorial for screen reader users"));
    setAccessibleDescription(
        tr("A step by step guide to running XFB with a screen reader and the keyboard."));
    setModal(false); // let the operator try things in the main window while reading
    resize(820, 560);

    buildTopics();
    buildUi();
    showTopic(0);
}

void AccessibilityTutorialDialog::buildTopics()
{
    // Keep every key sequence in sync with player::setupPlaybackShortcuts().
    const QString kPlay    = key(Qt::CTRL | Qt::SHIFT | Qt::Key_P);
    const QString kPause   = key(Qt::CTRL | Qt::SHIFT | Qt::Key_Space);
    const QString kStop    = key(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    const QString kNext    = key(Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    const QString kPrev    = key(Qt::CTRL | Qt::SHIFT | Qt::Key_B);
    const QString kWhat    = key(Qt::CTRL | Qt::SHIFT | Qt::Key_W);
    const QString kPrefs   = key(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    const QString kTime    = key(Qt::CTRL | Qt::Key_T);
    const QString kAddEnd  = key(Qt::CTRL | Qt::SHIFT | Qt::Key_Return);
    const QString kAddTop  = key(Qt::CTRL | Qt::ALT | Qt::Key_Return);

    m_topics.append({
        tr("Welcome"),
        tr("<h2>Welcome to XFB</h2>"
           "<p>XFB is radio automation software. It keeps a library of your music, "
           "lets you build a playlist, and plays that playlist to air.</p>"
           "<p>This tutorial is written for people who use a screen reader. Every task "
           "described here can be done with the keyboard alone.</p>"
           "<h3>Moving around this tutorial</h3>"
           "<ul>"
           "<li>Press Tab and Shift+Tab to move between the topic list, the reading area, "
           "and the buttons.</li>"
           "<li>In the topic list, use the Up and Down arrow keys to choose a topic.</li>"
           "<li>In this reading area, use the arrow keys to read line by line, and Page Up "
           "and Page Down to move faster.</li>"
           "<li>Press Alt+N for the next topic and Alt+P for the previous topic, from anywhere "
           "in this window.</li>"
           "<li>Press Escape to close the tutorial.</li>"
           "</ul>"
           "<p>The tutorial does not block the main window. You can leave it open and try "
           "each step as you read it.</p>")
    });

    m_topics.append({
        tr("Essential keyboard shortcuts"),
        tr("<h2>Essential keyboard shortcuts</h2>"
           "<p>These work anywhere in XFB, whatever currently has focus. They are also "
           "listed in the Playback menu.</p>"
           "<ul>"
           "<li><b>%1</b> — Start playing the playlist.</li>"
           "<li><b>%2</b> — Pause, or resume if already paused.</li>"
           "<li><b>%3</b> — Stop playback.</li>"
           "<li><b>%4</b> — Skip to the next track.</li>"
           "<li><b>%5</b> — Go back to the previous track.</li>"
           "<li><b>%6</b> — Say which track is playing right now.</li>"
           "<li><b>%7</b> — Say the playback time.</li>"
           "<li><b>%8</b> — Open Accessibility Preferences.</li>"
           "</ul>"
           "<h3>Building the running order</h3>"
           "<ul>"
           "<li><b>Enter</b> — Add the track you are on to the end of the playlist. This is "
           "the one you will use most.</li>"
           "<li><b>%9</b> — Add the selection to the end of the playlist from anywhere.</li>"
           "<li><b>%10</b> — Add the selection to the start, so it plays next.</li>"
           "</ul>"
           "<p>If you only remember three, make them <b>Enter</b> to build the running order, "
           "<b>%1</b> to go on air and <b>%6</b> to hear what is playing.</p>"
           "<h3>Reaching the menus</h3>"
           "<p>Use your screen reader's menu bar command to open the menus, then the arrow "
           "keys to move through them. The menus are: File, Playlists, Database, Playback, "
           "Options, Client, Server and Help.</p>")
            .arg(kPlay, kPause, kStop, kNext, kPrev, kWhat, kTime, kPrefs)
            .arg(kAddEnd, kAddTop)
    });

    m_topics.append({
        tr("How the window is arranged"),
        tr("<h2>How the window is arranged</h2>"
           "<p>Press Tab to move forward through the controls and Shift+Tab to move back. "
           "The main window holds two groups of tabbed pages.</p>"
           "<h3>The upper group</h3>"
           "<ul>"
           "<li><b>PlayList</b> — the running order. This is what goes to air.</li>"
           "<li><b>HistoryList</b> — what has already been played.</li>"
           "<li><b>DJ</b> — two decks for playing single files by hand.</li>"
           "</ul>"
           "<h3>The lower group</h3>"
           "<ul>"
           "<li><b>Musics</b> — your music library.</li>"
           "<li><b>Jingles</b> — short station idents.</li>"
           "<li><b>Pub</b> — adverts.</li>"
           "<li><b>Programs</b> — recorded or scheduled programmes.</li>"
           "<li><b>Torrents</b> — downloads, when that feature is switched on.</li>"
           "</ul>"
           "<p>Move between tabs with Control and Tab, or focus the tab bar and use the "
           "Left and Right arrow keys.</p>"
           "<p>The library and playlist are tables and lists. Use the arrow keys to move "
           "between rows; your screen reader will read each row as you land on it. The "
           "library columns are Artist, Song, Genre 1, Genre 2, Country, Published, File, "
           "Time, Plays and Last Played.</p>")
    });

    m_topics.append({
        tr("Adding music to the library"),
        tr("<h2>Adding music to the library</h2>"
           "<p>Everything you play comes from the library first. Open the <b>Database</b> "
           "menu and choose one of:</p>"
           "<ul>"
           "<li><b>Add a single song</b> — pick one file and fill in the details.</li>"
           "<li><b>Add all songs in a folder</b> — the fastest way to build a library from "
           "an existing music collection.</li>"
           "<li><b>Add a song from an external source</b> — download a track.</li>"
           "<li><b>Add a jingle</b>, <b>Add a publicity</b>, <b>Add a program</b> — for the "
           "other categories.</li>"
           "</ul>"
           "<p>XFB works out each track's length by itself, so the Time column fills in "
           "automatically. If a track shows no time, open the Database menu and choose "
           "<b>Check and Update Music Table Records</b> to read the lengths again.</p>"
           "<h3>Finding a track</h3>"
           "<p>Tab to the search box above the library, type part of an artist or song name, "
           "and the table below will show only the matching rows.</p>")
    });

    m_topics.append({
        tr("Building a playlist and going on air"),
        tr("<h2>Building a playlist and going on air</h2>"
           "<ol>"
           "<li>Go to the <b>Musics</b> tab and find the track you want with the arrow keys "
           "or the search box.</li>"
           "<li>Press <b>Enter</b> to add it to the end of the playlist. XFB says "
           "\"Added\" and the track name, so you know it worked.</li>"
           "<li>Repeat until the running order is the way you want it.</li>"
           "<li>Press <b>%1</b> to start playing.</li>"
           "</ol>"
           "<h3>Other ways to add tracks</h3>"
           "<ul>"
           "<li><b>%5</b> adds whatever is selected to the <i>end</i> of the playlist, from "
           "anywhere in the window.</li>"
           "<li><b>%6</b> adds it to the <i>start</i> instead, so it plays next.</li>"
           "<li>Select several rows first, with Shift and the arrow keys, to add them all "
           "in one go.</li>"
           "</ul>"
           "<p>The same keys work on the <b>Jingles</b> and <b>Programs</b> tabs.</p>"
           "<p>While playing:</p>"
           "<ul>"
           "<li><b>%2</b> pauses and resumes. XFB says \"Paused\" or \"Resumed\" so you always "
           "know which state you are in.</li>"
           "<li><b>%3</b> moves to the next track straight away.</li>"
           "<li><b>%4</b> tells you what is playing.</li>"
           "</ul>"
           "<p>Each time a new track starts, XFB announces \"Now playing\" followed by the "
           "track name, so you do not have to ask.</p>"
           "<h3>Saving a running order</h3>"
           "<p>Use the <b>Playlists</b> menu to Save, Load or Clear a playlist, so you can "
           "prepare a show in advance and load it when you go on air.</p>")
            .arg(kPlay, kPause, kNext, kWhat, kAddEnd, kAddTop)
    });

    m_topics.append({
        tr("Recording a programme"),
        tr("<h2>Recording a programme</h2>"
           "<p>Open the <b>Client</b> menu and choose <b>Record a new Program</b>.</p>"
           "<p>While recording, XFB keeps you informed without needing the screen:</p>"
           "<ul>"
           "<li>It says \"Recording paused\" and \"Recording resumed\" when you use the pause "
           "control.</li>"
           "<li>If the recording is producing no audio, XFB warns you out loud that the "
           "recording is empty and that you should check the input device. Do not ignore "
           "this — it means nothing is being captured.</li>"
           "</ul>"
           "<p>You can also turn the current playlist into a programme with <b>Make a program "
           "from this playlist</b> in the same menu.</p>")
    });

    m_topics.append({
        tr("Hearing what XFB is doing"),
        tr("<h2>Hearing what XFB is doing</h2>"
           "<p>XFB speaks through your screen reader. It announces:</p>"
           "<ul>"
           "<li>the track that has just started;</li>"
           "<li>pausing and resuming, for both playback and recording;</li>"
           "<li>warnings such as an empty recording.</li>"
           "</ul>"
           "<p>You can ask at any time:</p>"
           "<ul>"
           "<li><b>%1</b> — what is playing now.</li>"
           "<li><b>%2</b> — the playback time.</li>"
           "</ul>"
           "<h3>Controls that used to show state only by colour</h3>"
           "<p>Some buttons change colour to show their state. That state is now also part of "
           "the button's name, so your screen reader reads it. For example, the pause button "
           "is called \"Pause\" normally and \"Resume, paused\" once playback is paused. The "
           "recording indicator reads \"Recording\", \"Recording starting\", or \"Recording "
           "problem, nothing is being recorded\".</p>"
           "<h3>Adjusting how much XFB says</h3>"
           "<p>Press <b>%3</b>, or open the <b>Options</b> menu and choose <b>Accessibility "
           "Preferences</b>, to change how detailed the announcements are.</p>")
            .arg(kWhat, kTime, kPrefs)
    });

    m_topics.append({
        tr("If something does not work"),
        tr("<h2>If something does not work</h2>"
           "<h3>Nothing is being spoken</h3>"
           "<p>Check that your screen reader is running. XFB only speaks through it. Then "
           "press <b>%1</b> to open Accessibility Preferences and make sure announcements "
           "are switched on.</p>"
           "<h3>A shortcut does nothing</h3>"
           "<p>Some dialogs take over the keyboard while they are open. Close the dialog with "
           "Escape and try again.</p>"
           "<h3>A track will not play</h3>"
           "<p>The file may have been moved or deleted since it was added. Open the "
           "<b>Database</b> menu and choose <b>Check and Update Music Table Records</b>; XFB "
           "will check every file and offer to remove entries whose file is missing.</p>"
           "<h3>Getting this tutorial back</h3>"
           "<p>It is always in the <b>Help</b> menu, as <b>Tutorial for Blind Users</b>.</p>")
            .arg(kPrefs)
    });
}

void AccessibilityTutorialDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Use Tab to move between the topic list and the reading area. "
           "Alt+N is the next topic, Alt+P the previous one, Escape closes."), this);
    intro->setWordWrap(true);
    // Screen readers read a dialog's first focusable content; this line makes
    // the navigation model obvious before anything else is touched.
    intro->setAccessibleName(tr("How to use this tutorial"));
    outer->addWidget(intro);

    auto *split = new QHBoxLayout();

    m_topicList = new QListWidget(this);
    m_topicList->setAccessibleName(tr("Tutorial topics"));
    m_topicList->setAccessibleDescription(
        tr("Choose a topic with the up and down arrow keys."));
    for (const Topic &t : m_topics)
        m_topicList->addItem(t.title);
    m_topicList->setMaximumWidth(260);
    split->addWidget(m_topicList);

    m_content = new QTextBrowser(this);
    m_content->setAccessibleName(tr("Tutorial text"));
    m_content->setOpenExternalLinks(false);
    // Focusable with a caret so a screen reader can read it line by line
    // instead of only announcing it once as a block.
    m_content->setFocusPolicy(Qt::StrongFocus);
    m_content->setTextInteractionFlags(Qt::TextBrowserInteraction
                                       | Qt::TextSelectableByKeyboard);
    split->addWidget(m_content, 1);

    outer->addLayout(split, 1);

    m_positionLabel = new QLabel(this);
    m_positionLabel->setAccessibleName(tr("Position in the tutorial"));
    outer->addWidget(m_positionLabel);

    auto *buttons = new QHBoxLayout();
    auto *prev = new QPushButton(tr("&Previous topic"), this);
    auto *next = new QPushButton(tr("&Next topic"), this);
    auto *close = new QPushButton(tr("&Close"), this);
    prev->setAccessibleName(tr("Previous topic"));
    next->setAccessibleName(tr("Next topic"));
    close->setAccessibleName(tr("Close the tutorial"));
    buttons->addWidget(prev);
    buttons->addWidget(next);
    buttons->addStretch();
    buttons->addWidget(close);
    outer->addLayout(buttons);

    connect(prev, &QPushButton::clicked, this, &AccessibilityTutorialDialog::previousTopic);
    connect(next, &QPushButton::clicked, this, &AccessibilityTutorialDialog::nextTopic);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_topicList, &QListWidget::currentRowChanged,
            this, &AccessibilityTutorialDialog::showTopic);

    // Work from anywhere in the dialog, including while reading the text.
    auto *nextSc = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_N), this);
    nextSc->setContext(Qt::WindowShortcut);
    connect(nextSc, &QShortcut::activated, this, &AccessibilityTutorialDialog::nextTopic);

    auto *prevSc = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_P), this);
    prevSc->setContext(Qt::WindowShortcut);
    connect(prevSc, &QShortcut::activated, this, &AccessibilityTutorialDialog::previousTopic);

    close->setShortcut(QKeySequence::Cancel); // Escape
}

void AccessibilityTutorialDialog::showTopic(int index)
{
    if (index < 0 || index >= m_topics.size())
        return;

    const Topic &topic = m_topics.at(index);
    m_content->setHtml(topic.body);
    m_content->moveCursor(QTextCursor::Start); // start reading from the top

    // Name the reading area after the topic, so a screen reader identifies the
    // pane by what it currently contains rather than a generic label.
    m_content->setAccessibleName(tr("Tutorial text: %1").arg(topic.title));

    m_positionLabel->setText(tr("Topic %1 of %2: %3")
                                 .arg(index + 1)
                                 .arg(m_topics.size())
                                 .arg(topic.title));

    if (m_topicList->currentRow() != index)
        m_topicList->setCurrentRow(index);

    emit announcementRequested(tr("Topic %1 of %2: %3")
                                   .arg(index + 1)
                                   .arg(m_topics.size())
                                   .arg(topic.title));
}

void AccessibilityTutorialDialog::nextTopic()
{
    const int next = m_topicList->currentRow() + 1;
    if (next < m_topics.size())
        m_topicList->setCurrentRow(next);
    else
        emit announcementRequested(tr("This is the last topic"));
}

void AccessibilityTutorialDialog::previousTopic()
{
    const int prev = m_topicList->currentRow() - 1;
    if (prev >= 0)
        m_topicList->setCurrentRow(prev);
    else
        emit announcementRequested(tr("This is the first topic"));
}
