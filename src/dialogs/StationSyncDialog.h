#ifndef STATIONSYNCDIALOG_H
#define STATIONSYNCDIALOG_H

#include <QDialog>

#include "../services/StationSyncClient.h"

class MobileSyncServer;

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTimer;

/**
 * @brief The two ends of station-to-station backup in one window.
 *
 * A station that has never lost its studio machine will not go looking for
 * this, so the dialog is written to be understood cold: the top half is what
 * to do on the machine that is on air, the bottom half is what to do on the
 * machine standing by, and each says which is which. The same window is used
 * on both, because at the moment of setting it up nobody yet knows which
 * machine will turn out to be which.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h files in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class StationSyncDialog : public QDialog
{
    Q_OBJECT

public:
    StationSyncDialog(MobileSyncServer *server, StationSyncClient *client,
                      QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void toggleServer();
    void startPairing();
    void pairWithStation();
    void syncNow();
    void refresh();

private:
    void updateSourceSide();
    void updateBackupSide();

    MobileSyncServer  *m_server = nullptr;
    StationSyncClient *m_client = nullptr;

    // The station that is on air
    QLabel       *m_serverStatus = nullptr;
    QLabel       *m_serverAddress = nullptr;
    QLabel       *m_serverCode = nullptr;
    QPushButton  *m_serverToggle = nullptr;
    QPushButton  *m_serverPair = nullptr;
    QListWidget  *m_backupList = nullptr;

    // The station standing by
    QLineEdit    *m_peerHost = nullptr;
    QSpinBox     *m_peerPort = nullptr;
    QLineEdit    *m_peerCode = nullptr;
    QPushButton  *m_pairButton = nullptr;
    QPushButton  *m_forgetButton = nullptr;
    QLabel       *m_peerStatus = nullptr;
    QPushButton  *m_syncButton = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel       *m_progressText = nullptr;
    QCheckBox    *m_syncOnStart = nullptr;
    QSpinBox     *m_autoMinutes = nullptr;
    QLabel       *m_lastSync = nullptr;

    QTimer       *m_refreshTimer = nullptr;
};

#endif // STATIONSYNCDIALOG_H
