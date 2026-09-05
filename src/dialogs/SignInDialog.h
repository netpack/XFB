#ifndef SIGNINDIALOG_H
#define SIGNINDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;

/**
 * @brief Who is at the desk: the only way past a protected XFB.
 *
 * Two jobs, one window. At startup it asks who is signing on, and quitting is
 * the only other way out. Mid-show it is the lock screen: the same question,
 * except that the station is on air behind it and nothing about answering it
 * — or failing to — touches the audio. Whoever answers becomes the session,
 * so a presenter handing over to the next shift locks the screen and the
 * incoming presenter simply signs in over the top.
 *
 * Wrong answers are slowed down rather than counted: after a few failures the
 * button goes quiet for a few seconds and says so. A studio machine is behind
 * a locked door and the real risk is the person who wanders in, not a
 * password attack, so there is nothing here that can lock a station out of its
 * own playout at three in the morning.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h headers in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class SignInDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode {
        Startup,  ///< XFB is coming up; the alternative is quitting
        Lock,     ///< XFB is running (and possibly on air) behind this
    };

    explicit SignInDialog(Mode mode, QWidget *parent = nullptr);

    /** Ask, and keep asking until somebody signs in or gives up.
     *  @return true when a session was established. */
    static bool ask(Mode mode, QWidget *parent = nullptr);

protected:
    /** Escape must not dismiss the only thing standing in front of XFB. */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void attempt();
    void allowAnotherAttempt();

private:
    void buildUi();

    Mode m_mode;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QPushButton *m_signIn = nullptr;
    QLabel *m_message = nullptr;
    QTimer *m_cooldown = nullptr;
    int m_failures = 0;
};

#endif // SIGNINDIALOG_H
