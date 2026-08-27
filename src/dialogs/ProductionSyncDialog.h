#ifndef PRODUCTIONSYNCDIALOG_H
#define PRODUCTIONSYNCDIALOG_H

#include <QDialog>

#include "../services/ProductionSyncClient.h"

class MobileSyncServer;

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTimer;

/**
 * @brief The two ends of production-computer working, in one window.
 *
 * The top half is what to do on the machine that is on air: let a production
 * computer in, and see which ones have been let in already. The bottom half is
 * what to do on the production computer itself: point it at the station, fetch
 * what the station holds, and send back what has been prepared here.
 *
 * The same window serves both because a station rarely decides in advance
 * which machine is which, and because seeing both halves is what makes the
 * arrangement obvious: work flows down from the station and back up to it.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h files in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class ProductionSyncDialog : public QDialog
{
    Q_OBJECT

public:
    ProductionSyncDialog(MobileSyncServer *server, ProductionSyncClient *client,
                         QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void toggleServer();
    void startPairing();
    void pairWithStation();
    void fetchNow();
    void publishNow();
    void syncNow();
    void refresh();

private:
    void updateStationSide();
    void updateProductionSide();

    MobileSyncServer     *m_server = nullptr;
    ProductionSyncClient *m_client = nullptr;

    // The station that is on air
    QLabel      *m_serverStatus = nullptr;
    QLabel      *m_serverAddress = nullptr;
    QLabel      *m_serverCode = nullptr;
    QPushButton *m_serverToggle = nullptr;
    QPushButton *m_serverPair = nullptr;
    QListWidget *m_productionList = nullptr;
    QCheckBox   *m_allowDeletion = nullptr;
    QLabel      *m_incoming = nullptr;

    // The machine doing the preparing
    QLineEdit    *m_peerHost = nullptr;
    QSpinBox     *m_peerPort = nullptr;
    QLineEdit    *m_peerCode = nullptr;
    QPushButton  *m_pairButton = nullptr;
    QPushButton  *m_forgetButton = nullptr;
    QLabel       *m_peerStatus = nullptr;
    QPushButton  *m_fetchButton = nullptr;
    QPushButton  *m_publishButton = nullptr;
    QPushButton  *m_syncButton = nullptr;
    QListWidget  *m_pendingList = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel       *m_progressText = nullptr;
    QCheckBox    *m_syncOnStart = nullptr;
    QCheckBox    *m_publishAutomatically = nullptr;
    QSpinBox     *m_autoMinutes = nullptr;
    QLabel       *m_lastSync = nullptr;

    QTimer *m_refreshTimer = nullptr;
};

#endif // PRODUCTIONSYNCDIALOG_H
