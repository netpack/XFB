#ifndef MOBILESYNCDIALOG_H
#define MOBILESYNCDIALOG_H

#include <QDialog>

class MobileSyncServer;

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QTimer;

/**
 * @brief Operator-facing controls for the phone sync server: switch it on,
 *        show the pairing code, and see (or revoke) the devices that paired.
 *
 * Built in code rather than from a .ui file so there is no generated header to
 * regenerate — the committed ui_*.h files in src/ shadow AUTOUIC output, and a
 * stale one is a silent, confusing failure.
 */
class MobileSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MobileSyncDialog(MobileSyncServer *server, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void toggleServer();
    void startPairing();
    void revokeSelected();
    void clearMarkedTracks();
    void refresh();
    void updateRevokeButton();

private:
    void choosePlaylistsDirectory();
    void updateAddressChoices();
    void updateDeviceList();
    void updateMarkedTracks();
    void updatePairingCode();

    MobileSyncServer *m_server = nullptr;

    QLabel      *m_statusLabel = nullptr;
    QLabel      *m_addressLabel = nullptr;
    QLabel      *m_codeLabel = nullptr;
    QLabel      *m_qrLabel = nullptr;
    QComboBox   *m_addressBox = nullptr;
    QPushButton *m_toggleButton = nullptr;
    QPushButton *m_pairButton = nullptr;
    QPushButton *m_revokeButton = nullptr;
    QListWidget *m_deviceList = nullptr;
    QCheckBox   *m_autoStart = nullptr;
    QLabel      *m_markedLabel = nullptr;
    QPushButton *m_clearMarkedButton = nullptr;
    QLabel      *m_playlistsLabel = nullptr;
    QLabel      *m_companionLabel = nullptr;
    QTimer      *m_refreshTimer = nullptr;

    /// Redrawing the QR on every countdown tick would be wasteful and would
    /// make the image flicker, so it is only rebuilt when the URI changes.
    QString      m_shownUri;
};

#endif // MOBILESYNCDIALOG_H
