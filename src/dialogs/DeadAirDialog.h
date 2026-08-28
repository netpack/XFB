#ifndef DEADAIRDIALOG_H
#define DEADAIRDIALOG_H

#include <QDialog>

#include "../services/DeadAirWatchdog.h"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;

/**
 * @brief What the dead-air watchdog listens for, and what it puts on when it
 *        hears nothing.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h headers in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class DeadAirDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeadAirDialog(DeadAirWatchdog *watchdog, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void apply();
    void browsePlaylist();
    void browseFolder();
    void refreshStatus();
    void previewFallback();

private:
    void buildUi();
    void loadInto(const DeadAirWatchdog::Config &config);
    DeadAirWatchdog::Config readFromForm() const;

    DeadAirWatchdog *m_watchdog = nullptr;

    QCheckBox      *m_enabled = nullptr;
    QDoubleSpinBox *m_silenceDb = nullptr;
    QSpinBox       *m_tripSeconds = nullptr;
    QSpinBox       *m_recoverSeconds = nullptr;
    QSpinBox       *m_rearmSeconds = nullptr;
    QCheckBox      *m_watchStopped = nullptr;

    QLineEdit      *m_playlist = nullptr;
    QLineEdit      *m_folder = nullptr;
    QCheckBox      *m_shuffle = nullptr;
    QSpinBox       *m_trackCount = nullptr;
    QLabel         *m_preview = nullptr;

    QCheckBox      *m_notifyScreen = nullptr;
    QCheckBox      *m_notifyPhone = nullptr;

    QLabel         *m_status = nullptr;
    QLabel         *m_incident = nullptr;
    QPushButton    *m_clearButton = nullptr;

    QTimer         *m_refreshTimer = nullptr;
};

#endif // DEADAIRDIALOG_H
