#ifndef REMOTECONTROLDIALOG_H
#define REMOTECONTROLDIALOG_H

#include <QDialog>

class RemoteControlServer;

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTreeWidget;

/**
 * @brief Options ▸ Remote Control: the switch, the address, the keys, and what
 *        the keys have been doing.
 *
 * Built in code rather than from a .ui file, like the other network windows,
 * so there is no generated header in src/ to go stale.
 */
class RemoteControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteControlDialog(RemoteControlServer *server, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private:
    void setServerEnabled(bool enabled);
    /** A changed port or address only takes effect by listening again. */
    void restartIfRunning();
    void createKey();
    void revokeSelectedKey();
    void refresh();
    void refreshKeys();
    void addActivity(const QString &line);

    RemoteControlServer *m_server = nullptr;

    QCheckBox   *m_enabled = nullptr;
    QCheckBox   *m_webApp = nullptr;
    QComboBox   *m_bind = nullptr;
    QSpinBox    *m_port = nullptr;
    QLabel      *m_status = nullptr;
    QLabel      *m_example = nullptr;
    QLabel      *m_pageAddress = nullptr;
    QTreeWidget *m_keys = nullptr;
    QPushButton *m_revoke = nullptr;
    QListWidget *m_activity = nullptr;
    QString      m_lastError;
};

#endif // REMOTECONTROLDIALOG_H
