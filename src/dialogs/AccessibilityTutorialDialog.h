#ifndef ACCESSIBILITYTUTORIALDIALOG_H
#define ACCESSIBILITYTUTORIALDIALOG_H

#include <QDialog>
#include <QStringList>

class QListWidget;
class QTextBrowser;
class QLabel;

/**
 * @brief A keyboard-only, screen-reader oriented tutorial for XFB.
 *
 * Written for operators who cannot see the screen. Everything here is
 * reachable with the keyboard alone, the reading pane keeps focus so a screen
 * reader can walk the text line by line, and the key sequences shown are
 * generated from real QKeySequence values so they always match the running
 * platform (Command on macOS, Control elsewhere) and can never drift from the
 * shortcuts the application actually installs.
 */
class AccessibilityTutorialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessibilityTutorialDialog(QWidget *parent = nullptr);

signals:
    /** Emitted so the host window can speak a message through its announcer. */
    void announcementRequested(const QString &message);

private slots:
    void showTopic(int index);
    void nextTopic();
    void previousTopic();

private:
    void buildTopics();
    void buildUi();

    struct Topic {
        QString title;
        QString body; // simple HTML
    };

    QList<Topic> m_topics;
    QListWidget *m_topicList = nullptr;
    QTextBrowser *m_content = nullptr;
    QLabel *m_positionLabel = nullptr;
};

#endif // ACCESSIBILITYTUTORIALDIALOG_H
