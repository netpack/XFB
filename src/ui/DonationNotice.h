#ifndef DONATIONNOTICE_H
#define DONATIONNOTICE_H

#include <QWidget>

class QLabel;
class QTimer;
class QToolButton;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

/**
 * @brief The small "support XFB" reminder that replaces the startup donation
 *        dialog.
 *
 * The old reminder was a modal dialog thrown in front of the splash screen:
 * nobody could start working before dismissing it. This is the same message
 * as a toast — a child widget parked in the bottom-right corner of the main
 * window, above everything else but stealing neither focus nor the mouse.
 * It fades in, waits, and fades out on its own; the × closes it immediately
 * and the link opens the donation page. Playing a set is never interrupted.
 *
 * It follows the main window: the corner is recomputed whenever the parent
 * is resized, and the auto-hide countdown pauses while the pointer rests on
 * the notice so a reader is never cut off mid-sentence.
 *
 * @since XFB 3.1420
 */
class DonationNotice : public QWidget
{
    Q_OBJECT

public:
    /** The notice lives inside @p parent (the main window) and starts hidden. */
    explicit DonationNotice(QWidget *parent);

    /** Fades the notice in and (re)starts the auto-hide countdown. */
    void popUp();

    /** Fades the notice out; safe to call when it is already hidden. */
    void dismiss();

    /** The message a screen reader should hear when the notice appears. */
    QString spokenText() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void reposition();
    void applyPalette();
    void fadeTo(qreal opacity, int durationMs, bool hideWhenDone);

    QLabel *m_message = nullptr;
    QToolButton *m_close = nullptr;
    QTimer *m_autoHide = nullptr;
    QGraphicsOpacityEffect *m_opacity = nullptr;
    QPropertyAnimation *m_fade = nullptr;
};

#endif // DONATIONNOTICE_H
